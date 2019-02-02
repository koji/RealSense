#pragma once

// oF
#include "ofMain.h"
#include "ofxGuiExtended.h"

#include <librealsense2/rs.hpp>
#include <librealsense2/rs_advanced_mode.h> // advance_mode
#include "example.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <vector>


class ofApp : public ofBaseApp{
public:
    void setup();
    void update();
    void draw();
    
    void keyPressed(int key);
    
    void loadBagFile(string path);
    void exportPlyCloud(ofMesh pcMesh, string filename);
    
    // for exporting ply
    int ply_counter;

    rs2::pipeline pipe;
    rs2::pipeline_profile selection;
    rs2::device selected_device;
    rs2::device device;
    rs2::colorizer color_map;
    rs2::frame colored_depth;
    rs2::frame colored_filtered;
//    rs2::frame color;
    
    rs2::points points;
    rs2::pointcloud pc;
    
    // not needed
    glfw_state app_state;
    

    ofVboMesh mesh;
    ofEasyCam cam;
    bool saveFlg;
    
    // filter
    rs2::decimation_filter dec_filter;  // Decimation - reduces depth frame density
    rs2::spatial_filter spat_filter;    // Spatial    - edge-preserving spatial smoothing
    rs2::temporal_filter temp_filter;   // Temporal   - reduces temporal noise
    rs2::hole_filling_filter hole_filter;
    
    
    // disparity
    bool revert_disparity;
    const std::string disparity_filter_name = "Disparity";
//    rs2::disparity_transform depth_to_disparity();
//    rs2::disparity_transform disparity_to_depth();
    
    std::atomic_bool stopped(bool);
    
    // GUI
    ofxGui gui;
    ofxGuiPanel* panel;
    ofParameterGroup basic_parameters, decimate_parameters, spetial_parameters, temporal_parameters, hole_parameters;

    
    ofParameter<bool> usedDepth;
    ofParameter<float> alphaValue;
    ofParameter<float> scaleValue;
    ofParameter<float> gl_point_size;
    ofParameter<int> laser_power;
    int laser_min;
    int laser_max;
    
    // post processing
    // https://github.com/IntelRealSense/librealsense/blob/c3c758d18c585a237bb5b635927797aa69996391/doc/post-processing-filters.md
    
    ofParameter<int> decimate_magnitude;
    
    ofParameter<int> spatial_magnitude;
    ofParameter<float> spetial_smooth_alpha;
    ofParameter<int> spetial_smooth_delta;
    ofParameter<int> spetial_hole_filling;
    
    ofParameter<float> temporal_smooth_alpha;
    ofParameter<int> temporal_smooth_delta;
//    ofParameter<int> temporal_persistency_index;

    
    // checkbox
    ofParameter<bool> decimate;
    ofParameter<bool> spatial;
    ofParameter<bool> disparity;
    ofParameter<bool> temporal;
    ofParameter<bool> hole;
    
//    ofxGuiPanel *buttonPanel;
//    ofxGuiButton startBtn;
//    ofxGuiButton saveBtn;

};
