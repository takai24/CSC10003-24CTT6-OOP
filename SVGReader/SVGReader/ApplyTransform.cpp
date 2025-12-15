#include "stdafx.h"
#include "GdiPlusRenderer.h"

using namespace Gdiplus;

static std::vector<float> ParseNumbersLocal(const std::string &args)
{
    std::vector<float> numbers;
    std::string temp = args;
    for (char &c : temp)
        if (c == ',')
            c = ' ';

    std::stringstream ss(temp);
    float val;
    while (ss >> val)
    {
        numbers.push_back(val);
    }
    return numbers;
}

void GdiPlusRenderer::ApplyTransform(Graphics &graphics, const std::string &transformStr)
{
    if (transformStr.empty())
        return;

    std::regex re("([a-z]+)\\s*\\(([^)]+)\\)");
    std::sregex_iterator next(transformStr.begin(), transformStr.end(), re);
    std::sregex_iterator end;

    while (next != end)
    {
        std::smatch match = *next;
        std::string command = match[1];
        std::string args = match[2];

        std::vector<float> nums = ParseNumbersLocal(args);

        if (command == "translate" && nums.size() >= 1)
        {
            float tx = nums[0];
            float ty = (nums.size() >= 2) ? nums[1] : 0.0f;
            graphics.TranslateTransform(tx, ty);
        }
        else if (command == "rotate" && nums.size() >= 1)
        {
            float angle = nums[0];
            if (nums.size() >= 3)
            {
                float cx = nums[1];
                float cy = nums[2];
                graphics.TranslateTransform(cx, cy);
                graphics.RotateTransform(angle);
                graphics.TranslateTransform(-cx, -cy);
            }
            else
            {
                graphics.RotateTransform(angle);
            }
        }
        else if (command == "scale" && nums.size() >= 1)
        {
            float sx = nums[0];
            float sy = (nums.size() >= 2) ? nums[1] : sx;
            graphics.ScaleTransform(sx, sy);
        }

        next++;
    }
}
