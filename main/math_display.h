#ifndef MATH_DISPLAY_H
#define MATH_DISPLAY_H

#include <string>

// टूल को MCP सर्वर में रजिस्टर करने के लिए मुख्य फ़ंक्शन
void RegisterMathDisplayTool();

// स्क्रीन पर सॉल्यूशन दिखाने वाला फ़ंक्शन
void ShowMathSolutionOnScreen(const std::string& solution);

#endif // MATH_DISPLAY_H
