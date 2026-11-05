#include "application.h"
#include "assets.h"
#include "assets/lang_config.h"
#include "audio_codec.h"
#include "board.h"
#include "cjson_utils.h"
#include "display.h"
#include "mcp_server.h"
#include "mqtt_protocol.h"
#include "settings.h"
#include "system_info.h"
#include "text_glyph_payload.h"
#include "websocket_protocol.h"
#include "servo_controller.h"
#include "boot_anim.h"
#include "emo_mood.h"
#include "emo_touch.h"
#include "emo_espnow.h"

#include <driver/gpio.h>
#include <esp_log.h>
#include <arpa/inet.h>
#include <cJSON.h>
#include <algorithm>
#include <cstring>
#include <limits>
#include <esp_http_client.h>
#include <esp_crt_bundle.h>
#include <lwip/dns.h>
#include <esp_netif.h>
#include <esp_wifi.h>

#define TAG "Application"

Application::Application() : notify_player_(audio_service_) {
    event_group_ = xEventGroupCreate();

#if CONFIG_USE_DEVICE_AEC && CONFIG_USE_SERVER_AEC
#error "CONFIG_USE_DEVICE_AEC and CONFIG_USE_SERVER_AEC cannot be enabled at the same time"
#elif CONFIG_USE_DEVICE_AEC
    aec_mode_ = kAecOnDeviceSide;
#elif CONFIG_USE_SERVER_AEC
    aec_mode_ = kAecOnServerSide;
#else
    aec_mode_ = kAecOff;
#endif

    esp_timer_create_args_t clock_timer_args = {.callback =
                                                    [](void* arg) {
                                                        Application* app = (Application*)arg;
                                                        xEventGroupSetBits(app->event_group_,
                                                                           MAIN_EVENT_CLOCK_TICK);
                                                    },
                                                .arg = this,
                                                .dispatch_method = ESP_TIMER_TASK,
                                                .name = "clock_timer",
                                                .skip_unhandled_events = true};
    esp_timer_create(&clock_timer_args, &clock_timer_handle_);
}

Application::~Application() {
    notify_player_.Stop();
    if (clock_timer_handle_ != nullptr) {
        esp_timer_stop(clock_timer_handle_);
        esp_timer_delete(clock_timer_handle_);
    }
    if (startup_wake_word_hint_timer_ != nullptr) {
        esp_timer_stop(startup_wake_word_hint_timer_);
        esp_timer_delete(startup_wake_word_hint_timer_);
    }
    vEventGroupDelete(event_group_);
}

bool Application::SetDeviceState(DeviceState state) { return state_machine_.TransitionTo(state); }

void Application::Initialize() {
    auto& board = Board::GetInstance();
    SetDeviceState(kDeviceStateStarting);

    // Setup the display
    auto display = board.GetDisplay();
    display->SetupUI();
    RunEmoBootAnimation(display);
    EmoMoodEngine::GetInstance().Start(display);
    StartEmoTouchService(display);
  
    // Setup the audio service
    auto codec = board.GetAudioCodec();
    audio_service_.Initialize(codec);
    audio_service_.Start();
    ServoController::GetInstance().Initialize();
    ESP_LOGI(TAG, "After board/audio init");
    SystemInfo::PrintHeapStats();

    AudioServiceCallbacks callbacks;
    callbacks.on_send_queue_available = [this]() {
        xEventGroupSetBits(event_group_, MAIN_EVENT_SEND_AUDIO);
    };
    callbacks.on_wake_word_detected = [this](const std::string& wake_word) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_WAKE_WORD_DETECTED);
    };
    callbacks.on_vad_change = [this](bool speaking) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_VAD_CHANGE);
    };
    callbacks.on_playback_drained = [this]() {
        xEventGroupSetBits(event_group_, MAIN_EVENT_PLAYBACK_DRAINED);
    };
    callbacks.on_playback_progress = [this](uint32_t playback_id, uint32_t media_position_ms) {
        notify_player_.OnPlaybackProgress(playback_id, media_position_ms);
    };
    audio_service_.SetCallbacks(callbacks);

    // Add state change listeners
    state_machine_.AddStateChangeListener([this](DeviceState old_state, DeviceState new_state) {
        xEventGroupSetBits(event_group_, MAIN_EVENT_STATE_CHANGED);
    });

    // Start the clock timer to update the status bar
    esp_timer_start_periodic(clock_timer_handle_, 1000000);

    // Add MCP tools
    auto& mcp_server = McpServer::GetInstance();
    mcp_server.AddCommonTools();
    mcp_server.AddUserOnlyTools();

    // Math & Physics Equation Display Tool
    mcp_server.AddTool("show_math_solution",
        "Display step-by-step math or physics equation solutions on the screen",
        PropertyList({
            Property("solution", kPropertyTypeString)
        }),
        [this](const PropertyList& properties) -> ToolResult {
            std::string solution = properties["solution"].value<std::string>();
            auto display = Board::GetInstance().GetDisplay();
            if (display && !solution.empty()) {
                display->SetEmojiBoxVisible(false);
                display->ShowNotification(solution.c_str(), 15000);

                static esp_timer_handle_t restore_timer = nullptr;
                if (restore_timer != nullptr) {
                    esp_timer_stop(restore_timer);
                    esp_timer_delete(restore_timer);
                    restore_timer = nullptr;
                }

                esp_timer_create_args_t timer_args = {
                    .callback = [](void* arg) {
                        auto d = static_cast<Display*>(arg);
                        if (d) {
                            d->SetEmojiBoxVisible(true);
                        }
                    },
                    .arg = display,
                    .dispatch_method = ESP_TIMER_TASK,
                    .name = "restore_eyes",
                    .skip_unhandled_events = true
                };
                esp_timer_create(&timer_args, &restore_timer);
                esp_timer_start_once(restore_timer, 15000 * 1000);
            }
            return "Solution displayed on screen";
        }); 

    // 1. Light 1
    mcp_server.AddTool("control_light1", "Turn Light 1 on or off",
        PropertyList(),
        [](const PropertyList& properties) {
            SendRelayControl(DEVICE_LIGHT1, 1);
            return "Light 1 switched";
        });

    // 2. Light 2
    mcp_server.AddTool("control_light2", "Turn Light 2 on or off",
        PropertyList(),
        [](const PropertyList& properties) {
            SendRelayControl(DEVICE_LIGHT2, 1);
            return "Light 2 switched";
        });

    // 3. Fan
    mcp_server.AddTool("control_fan", "Turn Fan on or off",
        PropertyList(),
        [](const PropertyList& properties) {
            SendRelayControl(DEVICE_FAN, 1);
            return "Fan switched";
        });

    // 4. TV
    mcp_server.AddTool("control_tv", "Turn TV on or off",
        PropertyList(),
        [](const PropertyList& properties) {
            SendRelayControl(DEVICE_TV, 1);
            return "TV switched";
        });

    // 5. Play Song Tool (Direct High-Speed JioSaavn MP3 Engine)
    mcp_server.AddTool("play_song", "Play music or song by title",
        PropertyList({
            Property("query", kPropertyTypeString)
        }),
        [this](const PropertyList& properties) -> ToolResult {
            std::string query = properties["query"].value<std::string>();
            if (query.empty()) {
                query = "Alan Walker Faded";
            }

            ESP_LOGI("Music", "play_song tool triggered successfully with query: %s", query.c_str());

            xTaskCreate([](void* arg) {
                std::string* q_ptr = static_cast<std::string*>(arg);
                std::string raw_query = *q_ptr;
                delete q_ptr;

                // 1. WiFi Performance Mode
                esp_wifi_set_ps(WIFI_PS_NONE);

                // 2. Force DNS to Google & Cloudflare directly in lwIP
                ip_addr_t dns1, dns2;
                dns1.type = IPADDR_TYPE_V4;
                dns1.u_addr.ip4.addr = ipaddr_addr("8.8.8.8");
                dns_setserver(0, &dns1);

                dns2.type = IPADDR_TYPE_V4;
                dns2.u_addr.ip4.addr = ipaddr_addr("1.1.1.1");
                dns_setserver(1, &dns2);

                // 3. Clean Query format
                std::string clean_query = "";
                for (unsigned char c : raw_query) {
                    if (isalnum(c) || c == '-' || c == '_' || c == '.') {
                        clean_query += c;
                    } else if (c == ' ') {
                        clean_query += "+";
                    }
                }
                if (clean_query.empty()) clean_query = "Alan+Walker+Faded";

                std::string api_url = "https://saavn.dev/api/search/songs?query=" + clean_query + "&limit=1";
                ESP_LOGI("Music", "Fetching Saavn API: %s", api_url.c_str());

                esp_http_client_config_t config = {};
                config.url = api_url.c_str();
                config.timeout_ms = 10000;
                config.crt_bundle_attach = esp_crt_bundle_attach;
                config.buffer_size = 4096;
                config.buffer_size_tx = 1024;
                config.max_redirection_count = 5;
                config.skip_cert_common_name_check = true;

                esp_http_client_handle_t client = esp_http_client_init(&config);
                esp_http_client_set_header(client, "User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64)");
                esp_http_client_set_header(client, "Accept", "application/json");

                esp_err_t err = esp_http_client_open(client, 0);
                std::string audio_url = "";
                std::string track_name = raw_query;

                if (err == ESP_OK) {
                    esp_http_client_fetch_headers(client);
                    int status_code = esp_http_client_get_status_code(client);
                    ESP_LOGI("Music", "Saavn Status = %d", status_code);

                    if (status_code == 200) {
                        std::string res = "";
                        char chunk[1024];
                        int r = 0;
                        while ((r = esp_http_client_read(client, chunk, sizeof(chunk) - 1)) > 0) {
                            chunk[r] = '\0';
                            res += chunk;
                            if (res.length() > 24576) break;
                        }

                        cJSON* root = cJSON_Parse(res.c_str());
                        if (root) {
                            cJSON* data = cJSON_GetObjectItem(root, "data");
                            cJSON* results = data ? cJSON_GetObjectItem(data, "results") : nullptr;
                            if (results && cJSON_IsArray(results) && cJSON_GetArraySize(results) > 0) {
                                cJSON* song = cJSON_GetArrayItem(results, 0);
                                cJSON* name = cJSON_GetObjectItem(song, "name");
                                if (name && cJSON_IsString(name)) {
                                    track_name = name->valuestring;
                                }

                                cJSON* download_url = cJSON_GetObjectItem(song, "downloadUrl");
                                if (download_url && cJSON_IsArray(download_url)) {
                                    int count = cJSON_GetArraySize(download_url);
                                    for (int i = 0; i < count; i++) {
                                        cJSON* item = cJSON_GetArrayItem(download_url, i);
                                        cJSON* quality = cJSON_GetObjectItem(item, "quality");
                                        cJSON* url = cJSON_GetObjectItem(item, "url");
                                        if (quality && url && cJSON_IsString(url)) {
                                            audio_url = url->valuestring;
                                            if (strcmp(quality->valuestring, "160kbps") == 0 ||
                                                strcmp(quality->valuestring, "96kbps") == 0) {
                                                break;
                                            }
                                        }
                                    }
                                }
                            }
                            cJSON_Delete(root);
                        }
                    }
                } else {
                    ESP_LOGE("Music", "HTTP open failed: %s", esp_err_to_name(err));
                }
                esp_http_client_cleanup(client);

                if (!audio_url.empty()) {
                    auto& app = Application::GetInstance();
                    vTaskDelay(pdMS_TO_TICKS(1500));

                    app.Schedule([&app]() {
                        app.SetDeviceState(kDeviceStateIdle);
                    });

                    vTaskDelay(pdMS_TO_TICKS(200));

                    ESP_LOGI("Music", "Playing Saavn MP3: %s (%s)", track_name.c_str(), audio_url.c_str());
                    app.StartNotification(audio_url, {});
                } else {
                    ESP_LOGW("Music", "Song not found on Saavn");
                }

                vTaskDelete(NULL);
            }, "music_task", 10240, new std::string(query), 4, NULL);

            return "Playing " + query;
        });

    // Set network event callback for UI updates and network state handling
    board.SetNetworkEventCallback([this](NetworkEvent event, const std::string& data) {
        auto display = Board::GetInstance().GetDisplay();

        switch (event) {
            case NetworkEvent::Scanning:
                xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_DISCONNECTED);
                break;
            case NetworkEvent::Connecting:
                break;
            case NetworkEvent::Connected:
                xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_CONNECTED);
                break;
            case NetworkEvent::Disconnected:
                xEventGroupSetBits(event_group_, MAIN_EVENT_NETWORK_DISCONNECTED);
                break;
            case NetworkEvent::WifiConfigModeEnter:
            case NetworkEvent::WifiConfigModeExit:
                break;
            case NetworkEvent::ModemDetecting:
                display->SetStatus(Lang::Strings::DETECTING_MODULE);
                break;
            case NetworkEvent::ModemErrorNoSim:
                Alert(Lang::Strings::ERROR, Lang::Strings::PIN_ERROR, "warning",
                      Lang::Sounds::OGG_ERR_PIN);
                break;
            case NetworkEvent::ModemErrorRegDenied:
                Alert(Lang::Strings::ERROR, Lang::Strings::REG_ERROR, "warning",
                      Lang::Sounds::OGG_ERR_REG);
                break;
            case NetworkEvent::ModemErrorInitFailed:
                Alert(Lang::Strings::ERROR, Lang::Strings::MODEM_INIT_ERROR, "warning",
                      Lang::Sounds::OGG_EXCLAMATION);
                break;
            case NetworkEvent::ModemErrorTimeout:
                display->SetStatus(Lang::Strings::REGISTERING_NETWORK);
                break;
        }
    });

    // Start network asynchronously
    board.StartNetwork();

    // Update the status bar immediately to show the network state
    display->UpdateStatusBar(true);
}

void Application::Run() {
    vTaskPrioritySet(nullptr, 10);

    const EventBits_t ALL_EVENTS =
        MAIN_EVENT_SCHEDULE | MAIN_EVENT_SEND_AUDIO | MAIN_EVENT_WAKE_WORD_DETECTED |
        MAIN_EVENT_VAD_CHANGE | MAIN_EVENT_CLOCK_TICK | MAIN_EVENT_ERROR |
        MAIN_EVENT_NETWORK_CONNECTED | MAIN_EVENT_NETWORK_DISCONNECTED | MAIN_EVENT_TOGGLE_CHAT |
        MAIN_EVENT_START_LISTENING | MAIN_EVENT_STOP_LISTENING | MAIN_EVENT_ACTIVATION_DONE |
        MAIN_EVENT_STATE_CHANGED | MAIN_EVENT_PLAYBACK_DRAINED | MAIN_EVENT_STARTUP_HINT_TIMEOUT;

    while (true) {
        auto bits = xEventGroupWaitBits(event_group_, ALL_EVENTS, pdTRUE, pdFALSE, portMAX_DELAY);

        if (bits & MAIN_EVENT_ERROR) {
            if (GetDeviceState() == kDeviceStateNotifying) {
                StopNotification();
            }
            SetDeviceState(kDeviceStateIdle);
            Alert(Lang::Strings::ERROR, last_error_message_.c_str(), "cancel",
                  Lang::Sounds::OGG_EXCLAMATION);
        }

        if (bits & MAIN_EVENT_NETWORK_CONNECTED) {
            HandleNetworkConnectedEvent();
        }

        if (bits & MAIN_EVENT_NETWORK_DISCONNECTED) {
            HandleNetworkDisconnectedEvent();
        }

        if (bits & MAIN_EVENT_ACTIVATION_DONE) {
            HandleActivationDoneEvent();
        }

        if (bits & MAIN_EVENT_STATE_CHANGED) {
            HandleStateChangedEvent();
        }

        if (bits & MAIN_EVENT_PLAYBACK_DRAINED) {
            if (audio_service_.IsPlaybackIdle()) {
                notify_player_.OnPlaybackDrained();
            }
            if (pending_listening_start_ && GetDeviceState() == kDeviceStateListening &&
                audio_service_.IsPlaybackIdle()) {
                pending_listening_start_ = false;
                StartListeningAudio();
            }
        }

        if (bits & MAIN_EVENT_TOGGLE_CHAT) {
            HandleToggleChatEvent();
        }

        if (bits & MAIN_EVENT_START_LISTENING) {
            HandleStartListeningEvent();
        }

        if (bits & MAIN_EVENT_STOP_LISTENING) {
            HandleStopListeningEvent();
        }

        if (bits & MAIN_EVENT_SEND_AUDIO) {
            while (auto packet = audio_service_.PopPacketFromSendQueue()) {
                if (protocol_ && !protocol_->SendAudio(std::move(packet))) {
                    while (audio_service_.PopPacketFromSendQueue())
                        ;
                    break;
                }
            }
        }

        if (bits & MAIN_EVENT_WAKE_WORD_DETECTED) {
            HandleWakeWordDetectedEvent();
        }

        if (bits & MAIN_EVENT_VAD_CHANGE) {
            if (GetDeviceState() == kDeviceStateListening) {
                auto led = Board::GetInstance().GetLed();
                led->OnStateChanged();
            }
        }

        if (bits & MAIN_EVENT_SCHEDULE) {
            std::unique_lock<std::mutex> lock(mutex_);
            auto tasks = std::move(main_tasks_);
            lock.unlock();
            for (auto& task : tasks) {
                task();
            }
        }

        if (bits & MAIN_EVENT_CLOCK_TICK) {
            clock_ticks_++;
            auto display = Board::GetInstance().GetDisplay();
            display->UpdateStatusBar();
            if (GetDeviceState() == kDeviceStateIdle) {
                ServoController::GetInstance().OnIdleTick();
            } else if (GetDeviceState() == kDeviceStateSpeaking) {
                ServoController::GetInstance().OnSpeakingTick();
            }

            if (clock_ticks_ % 10 == 0) {
                SystemInfo::PrintHeapStats();
            }
        }
        if (bits & MAIN_EVENT_STARTUP_HINT_TIMEOUT) {
            HideStartupWakeWordHint();
        }
        ShowStartupWakeWordHint();
    }
}

void Application::HandleNetworkConnectedEvent() {
    ESP_LOGI(TAG, "Network connected");

    esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (netif) {
        esp_netif_dns_info_t dns_info = {};
        dns_info.ip.u_addr.ip4.addr = ipaddr_addr("8.8.8.8");
        dns_info.ip.type = ESP_IPADDR_TYPE_V4;
        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_MAIN, &dns_info);

        dns_info.ip.u_addr.ip4.addr = ipaddr_addr("1.1.1.1");
        esp_netif_set_dns_info(netif, ESP_NETIF_DNS_BACKUP, &dns_info);
        ESP_LOGI(TAG, "Custom DNS 8.8.8.8 and 1.1.1.1 applied successfully");
    }

    auto state = GetDeviceState();
    InitEmoRelaySender();
   
    if (state == kDeviceStateStarting || state == kDeviceStateWifiConfiguring) {
        SetDeviceState(kDeviceStateActivating);
        if (activation_task_handle_ != nullptr) {
            ESP_LOGW(TAG, "Activation task already running");
            return;
        }

        xTaskCreate(
            [](void* arg) {
                Application* app = static_cast<Application*>(arg);
                app->ActivationTask();
                app->activation_task_handle_ = nullptr;
                vTaskDelete(NULL);
            },
            "activation", 4096 * 2, this, 2, &activation_task_handle_);
    }

    auto display = Board::GetInstance().GetDisplay();
    display->UpdateStatusBar(true);
}

void Application::HandleNetworkDisconnectedEvent() {
    auto state = GetDeviceState();
    if (state == kDeviceStateNotifying) {
        StopNotification();
    }
    if (state == kDeviceStateConnecting || state == kDeviceStateListening ||
        state == kDeviceStateSpeaking) {
        ESP_LOGI(TAG, "Closing audio channel due to network disconnection");
        protocol_->CloseAudioChannel();
    }

    auto display = Board::GetInstance().GetDisplay();
    display->UpdateStatusBar(true);
}

void Application::HandleActivationDoneEvent() {
    ESP_LOGI(TAG, "Activation done");

    SystemInfo::PrintHeapStats();
    SetDeviceState(kDeviceStateIdle);

    has_server_time_ = ota_->HasServerTime();

    const bool has_error = !last_error_message_.empty();
    if (!startup_completed_) {
        startup_completed_ = true;
        pending_startup_wake_word_hint_ = !has_error;
    }
  
    ota_.reset();
    auto& board = Board::GetInstance();
    board.SetPowerSaveLevel(PowerSaveLevel::LOW_POWER);

    if (!has_error) {
        Schedule([this]() {
            audio_service_.PlaySound(Lang::Sounds::OGG_SUCCESS);
        });
    }
}

void Application::ActivationTask() {
    ota_ = std::make_unique<Ota>();
    CheckAssetsVersion();
    CheckNewVersion();
    InitializeProtocol();
    xEventGroupSetBits(event_group_, MAIN_EVENT_ACTIVATION_DONE);
}

void Application::CheckAssetsVersion() {
    if (assets_version_checked_) {
        return;
    }
    assets_version_checked_ = true;

    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();
    auto& assets = Assets::GetInstance();

    if (!assets.partition_valid()) {
        ESP_LOGW(TAG, "Assets partition is disabled for board %s", BOARD_NAME);
        return;
    }

    Settings settings("assets", true);
    std::string download_url = settings.GetString("download_url");

    if (!download_url.empty()) {
        settings.EraseKey("download_url");

        char message[256];
        snprintf(message, sizeof(message), Lang::Strings::FOUND_NEW_ASSETS, download_url.c_str());
        Alert(Lang::Strings::LOADING_ASSETS, message, "cloud_download", Lang::Sounds::OGG_UPGRADE);

        vTaskDelay(pdMS_TO_TICKS(3000));
        SetDeviceState(kDeviceStateUpgrading);
        board.SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);
        display->SetChatMessage("system", Lang::Strings::PLEASE_WAIT);

        bool success =
            assets.Download(download_url, [this, display](int progress, size_t speed) -> void {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%d%% %uKB/s", progress, speed / 1024);
                Schedule([display, message = std::string(buffer)]() {
                    display->SetChatMessage("system", message.c_str());
                });
            });

        board.SetPowerSaveLevel(PowerSaveLevel::LOW_POWER);
        vTaskDelay(pdMS_TO_TICKS(1000));

        if (!success) {
            Alert(Lang::Strings::ERROR, Lang::Strings::DOWNLOAD_ASSETS_FAILED, "cancel",
                  Lang::Sounds::OGG_EXCLAMATION);
            vTaskDelay(pdMS_TO_TICKS(2000));
            SetDeviceState(kDeviceStateActivating);
            return;
        }
    }

    assets.Apply();
    display->SetChatMessage("system", "");
}

void Application::CheckNewVersion() {
    const int MAX_RETRY = 10;
    int retry_count = 0;
    int retry_delay = 10;

    auto& board = Board::GetInstance();
    while (true) {
        auto display = board.GetDisplay();
        auto check = ota_->CheckVersion();
        if (!check) {
            retry_count++;
            if (retry_count >= MAX_RETRY) {
                ESP_LOGE(TAG, "Too many retries, exit version check");
                return;
            }

            const auto& err = check.error();
            char error_message[160];
            int error_message_length =
                snprintf(error_message, sizeof(error_message), "%s", err.ToString().c_str());
            if (error_message_length < 0 ||
                error_message_length >= static_cast<int>(sizeof(error_message))) {
                snprintf(error_message, sizeof(error_message), "%s", err.Message());
            }

            char buffer[320];
            int alert_message_length =
                snprintf(buffer, sizeof(buffer), Lang::Strings::CHECK_NEW_VERSION_FAILED,
                         retry_delay, error_message);
            if (alert_message_length < 0 ||
                alert_message_length >= static_cast<int>(sizeof(buffer))) {
                snprintf(buffer, sizeof(buffer), "%s", err.Message());
            }
            Alert(Lang::Strings::ERROR, buffer, "cloud_off", Lang::Sounds::OGG_EXCLAMATION);

            ESP_LOGW(TAG, "Check new version failed, retry in %d seconds (%d/%d)", retry_delay,
                     retry_count, MAX_RETRY);
            for (int i = 0; i < retry_delay; i++) {
                vTaskDelay(pdMS_TO_TICKS(1000));
                if (GetDeviceState() == kDeviceStateIdle) {
                    break;
                }
            }
            retry_delay *= 2;
            continue;
        }
        retry_count = 0;
        retry_delay = 10;

        if (ota_->HasNewVersion()) {
            if (UpgradeFirmware(ota_->GetFirmwareUrl(), ota_->GetFirmwareVersion())) {
                return;
            }
        }

        ota_->MarkCurrentVersionValid();
        if (!ota_->HasActivationCode() && !ota_->HasActivationChallenge()) {
            break;
        }

        display->SetStatus(Lang::Strings::ACTIVATION);
        if (ota_->HasActivationCode()) {
            ShowActivationCode(ota_->GetActivationCode(), ota_->GetActivationMessage());
        }

        for (int i = 0; i < 10; ++i) {
            ESP_LOGI(TAG, "Activating... %d/%d", i + 1, 10);
            esp_err_t err = ota_->Activate();
            if (err == ESP_OK) {
                break;
            } else if (err == ESP_ERR_TIMEOUT) {
                vTaskDelay(pdMS_TO_TICKS(3000));
            } else {
                vTaskDelay(pdMS_TO_TICKS(10000));
            }
            if (GetDeviceState() == kDeviceStateIdle) {
                break;
            }
        }
    }
}

void Application::InitializeProtocol() {
    auto& board = Board::GetInstance();
    auto display = board.GetDisplay();
    auto codec = board.GetAudioCodec();

    display->SetStatus(Lang::Strings::LOADING_PROTOCOL);

    if (ota_->HasMqttConfig()) {
        protocol_ = std::make_unique<MqttProtocol>();
    } else if (ota_->HasWebsocketConfig()) {
        protocol_ = std::make_unique<WebsocketProtocol>();
    } else {
        ESP_LOGW(TAG, "No protocol specified in the OTA config, using MQTT");
        protocol_ = std::make_unique<MqttProtocol>();
    }

    protocol_->OnConnected([this]() { DismissAlert(); });

    protocol_->OnNetworkError([this](const std::string& message) {
        last_error_message_ = message;
        xEventGroupSetBits(event_group_, MAIN_EVENT_ERROR);
    });

    protocol_->OnIncomingAudio([this](std::unique_ptr<AudioStreamPacket> packet) {
        if (GetDeviceState() == kDeviceStateSpeaking) {
            audio_service_.PushPacketToDecodeQueue(std::move(packet));
        }
    });

    protocol_->OnAudioChannelOpened([this, codec, &board]() {
        board.SetPowerSaveLevel(PowerSaveLevel::PERFORMANCE);
        if (protocol_->server_sample_rate() != codec->output_sample_rate()) {
            ESP_LOGW(TAG,
                     "Server sample rate %d does not match device output sample rate %d, "
                     "resampling may cause distortion",
                     protocol_->server_sample_rate(), codec->output_sample_rate());
        }
    });

    protocol_->OnAudioChannelClosed([this, &board]() {
        board.SetPowerSaveLevel(PowerSaveLevel::LOW_POWER);
        Schedule([this]() {
            auto display = Board::GetInstance().GetDisplay();
            display->SetChatMessage("system", "");
            SetDeviceState(kDeviceStateIdle);
        });
    });

    protocol_->OnIncomingJson([this, display](const cJSON* root) {
        auto type = cJSON_GetObjectItem(root, "type");
        if (!cJSON_IsString(type)) {
            ESP_LOGW(TAG, "Incoming JSON message has no type");
            return;
        }
        if (strcmp(type->valuestring, "notify") == 0) {
            auto audio_url = cJSON_GetObjectItem(root, "audio_url");
            if (!cJSON_IsString(audio_url) || audio_url->valuestring[0] == '\0') {
                ESP_LOGW(TAG, "Notify message requires audio_url");
                return;
            }

            std::vector<NotifySubtitle> subtitles;
            auto subtitles_json = cJSON_GetObjectItem(root, "subtitles");
            if (subtitles_json != nullptr && !cJSON_IsArray(subtitles_json)) {
                ESP_LOGW(TAG, "Notify subtitles must be an array");
                return;
            }
            if (cJSON_IsArray(subtitles_json)) {
                cJSON* item = nullptr;
                cJSON_ArrayForEach (item, subtitles_json) {
                    auto start_ms = cJSON_GetObjectItem(item, "start_ms");
                    auto text = cJSON_GetObjectItem(item, "text");
                    if (!cJSON_IsNumber(start_ms) || start_ms->valuedouble < 0 ||
                        start_ms->valuedouble > std::numeric_limits<uint32_t>::max() ||
                        !cJSON_IsString(text)) {
                        ESP_LOGW(TAG, "Ignoring invalid notify subtitle");
                        continue;
                    }
                    subtitles.push_back({.start_ms = static_cast<uint32_t>(start_ms->valuedouble),
                                         .text = text->valuestring});
                }
            }

            Schedule([this, url = std::string(audio_url->valuestring),
                      subtitles = std::move(subtitles)]() mutable {
                StartNotification(std::move(url), std::move(subtitles));
            });
        } else if (strcmp(type->valuestring, "tts") == 0) {
            auto state = cJSON_GetObjectItem(root, "state");
            if (!cJSON_IsString(state)) {
                return;
            }
            if (strcmp(state->valuestring, "start") == 0) {
                Schedule([this]() {
                    aborted_ = false;
                    SetDeviceState(kDeviceStateSpeaking);
                });
            } else if (strcmp(state->valuestring, "stop") == 0) {
                Schedule([this]() {
                    if (GetDeviceState() ==
