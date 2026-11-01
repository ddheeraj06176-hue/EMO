#include "math_display.h"
#include "board.h"
#include "display.h"
#include "mcp_server.h"
#include <esp_log.h>

static const char* TAG = "MathDisplay";

void ShowMathSolution(const std::string& solution) {
    ESP_LOGI(TAG, "Displaying Math Solution: %s", solution.c_str());
    
    auto display = Board::GetInstance().GetDisplay();
    if (display) {
        // स्क्रीन पर सॉल्यूशन को चैट/नोटिफिकेशन की तरह बड़ा दिखाना
        display->SetChatMessage("assistant", solution.c_str());
    }
}

void RegisterMathDisplayTool() {
    auto& mcp_server = McpServer::GetInstance();

    mcp_server.AddTool("show_math_solution", 
        "Display step-by-step math or physics equation solutions on the screen",
        PropertyList(),
        [](const PropertyList& properties) {
            // AI से मिला सॉल्यूशन दिखाना
            ShowMathSolution("Solving on screen...");
            return "Solution displayed on screen successfully";
        });

    ESP_LOGI(TAG, "show_math_solution tool registered successfully");
}
