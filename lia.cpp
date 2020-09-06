#include "lia.h"
std::string detect_lia_mode(std::string m_HCLCommandString){
    std::string lia_map_mode ="Map mode: ";
        if (m_HCLCommandString.find("v")!=std::string::npos){
            lia_map_mode = lia_map_mode + "Выживание ";
        }
        if (m_HCLCommandString.find("x")!=std::string::npos){
            lia_map_mode = lia_map_mode + "Экстрим ";
        }
        if (m_HCLCommandString.find("c")!=std::string::npos){
            lia_map_mode = lia_map_mode + "Случайные герои  ";
        }
        if (m_HCLCommandString.find("e")!=std::string::npos){
            lia_map_mode = lia_map_mode + "Легкий ";
        }
        if (m_HCLCommandString.find("b")!=std::string::npos){
            lia_map_mode = lia_map_mode + "Битва кланов ";
        }
        if (m_HCLCommandString.find("z")!=std::string::npos){
            lia_map_mode = lia_map_mode + "Золото поровну ";
        }
        if (m_HCLCommandString ==""){
            lia_map_mode = "Выживание";
        }
        return lia_map_mode;
    }