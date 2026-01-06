#include "stdafx.h"
#include "SvgColors.h"

std::unordered_map<std::string, Color> SvgColors::s_colorMap;

void SvgColors::LoadColors(const std::string& filePath) {
    ifstream color_file(filePath, ios::in);

    if (!color_file.is_open()) {
        s_colorMap["none"] = Color(0, 0, 0, 0);
        s_colorMap["black"] = Color(255, 0, 0, 0);
        s_colorMap["white"] = Color(255, 255, 255, 255);
        return;
    }

    string color_line = "";
    while (getline(color_file, color_line)) {
        if (color_line.empty()) continue;

        stringstream ss(color_line);
        string token = "";
        vector<string> vct;

        while (ss >> token)
            vct.push_back(token);

        if (vct.size() < 2) continue; 

        string color_name = "";
        string hexa_code;
        int n = vct.size();

        
        for (int i = 0; i < n - 1; i++)
            color_name = color_name + vct[i];

        
        transform(color_name.begin(), color_name.end(), color_name.begin(), ::tolower);

        hexa_code = vct[n - 1]; 

        try {
            if (!hexa_code.empty() && hexa_code[0] == '#') {
                if (hexa_code.size() == 4) { // #RGB
                    string r_s = string(2, hexa_code[1]);
                    string g_s = string(2, hexa_code[2]);
                    string b_s = string(2, hexa_code[3]);
                    int r = stoi(r_s, nullptr, 16);
                    int g = stoi(g_s, nullptr, 16);
                    int b = stoi(b_s, nullptr, 16);
                    s_colorMap[color_name] = Color(255, r, g, b);
                }
                else if (hexa_code.size() >= 7) { // #RRGGBB
                    int r = stoi(hexa_code.substr(1, 2), nullptr, 16);
                    int g = stoi(hexa_code.substr(3, 2), nullptr, 16);
                    int b = stoi(hexa_code.substr(5, 2), nullptr, 16);
                    s_colorMap[color_name] = Color(255, r, g, b);
                }
            }
        }
        catch (...) {
            continue; 
        }
    }

    s_colorMap["none"] = Color(0, 0, 0, 0);
    s_colorMap["transparent"] = Color(0, 0, 0, 0);
    color_file.close();
}

Color SvgColors::GetColor(std::string strokecolor, std::string strokeopa) {

    float opacityVal = 1.0f;
    if (!strokeopa.empty()) {
        try { opacityVal = stof(strokeopa); }
        catch (...) { opacityVal = 1.0f; }
    }
    if (opacityVal > 1.0f) opacityVal = 1.0f;
    if (opacityVal < 0.0f) opacityVal = 0.0f;

    BYTE alpha = (BYTE)(opacityVal * 255); 

    
    if (strokecolor.find("rgb") != string::npos) {
        for (int i = 0; i < strokecolor.size(); i++) {
            if (!isdigit(strokecolor[i]) && strokecolor[i] != '.')
                strokecolor[i] = ' ';
        }

        stringstream ss(strokecolor);
        float r = 0, g = 0, b = 0;
        ss >> r >> g >> b;

        return Color(alpha, (BYTE)r, (BYTE)g, (BYTE)b);
    }

    else if (!strokecolor.empty() && strokecolor[0] == '#') {
        if (strokecolor.size() == 4) { // Convert #RGB -> #RRGGBB
            string tmp = "#";
            for (int i = 1; i < 4; i++)
                tmp = tmp + strokecolor[i] + strokecolor[i];
            strokecolor = tmp;
        }

        try {
            int r = stoi(strokecolor.substr(1, 2), nullptr, 16);
            int g = stoi(strokecolor.substr(3, 2), nullptr, 16);
            int b = stoi(strokecolor.substr(5, 2), nullptr, 16);
            return Color(alpha, r, g, b);
        }
        catch (...) {
            return Color(0, 0, 0, 0); 
        }
    }

    
    else {
        
        string key = strokecolor;
        for (int i = 0; i < key.size(); i++) {
            if (isupper(key[i])) key[i] = tolower(key[i]);
        }

        key.erase(0, key.find_first_not_of(" \t\r\n"));
        key.erase(key.find_last_not_of(" \t\r\n") + 1);

        string searchKey = "";
        for (char c : key) {
            if (c != ' ') searchKey += c;
        }

        if (s_colorMap.find(searchKey) != s_colorMap.end()) {
            Color baseColor = s_colorMap[searchKey];
            return Color(alpha, baseColor.GetR(), baseColor.GetG(), baseColor.GetB());
        }
    }

    return Color(0, 0, 0, 0);
}