//
//  Filter.h
//  ofScanner
//
//  Created by Koji Kanao on 11/28/18.
//

#pragma once

#ifndef Filter_h
#define Filter_h
#include <librealsense2/rs.hpp>
#include "example.hpp"


//struct filter_slider_ui
//{
//    std::string name;
//    std::string label;
//    std::string description;
//    bool is_int;
//    float value;
//    rs2::option_range range;
//
//    bool render(const float3& location, bool enabled);
//    static bool is_all_integers(const rs2::option_range& range);
//};

/**
 Class to encapsulate a filter alongside its options
 */
class filter_options
{
public:
    filter_options(const std::string name, rs2::process_interface& filter);
    filter_options(filter_options&& other);
    std::string filter_name;                                   //Friendly name of the filter
    rs2::process_interface& filter;                                       //The filter in use
//    std::map<rs2_option, filter_slider_ui> supported_options;  //maps from an option supported by the filter, to the corresponding slider
    std::atomic_bool is_enabled;                               //A boolean controlled by the user that determines whether to apply the filter or not
};



#endif /* Filter_h */
