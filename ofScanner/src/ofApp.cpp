#include "ofApp.h"
#define GLFW_INCLUDE_GLU
#include <GLFW/glfw3.h>
#include <algorithm>  // for std::min, std::max


// post-processing
#include <map>
#include <string>
#include <thread>
#include <atomic>

//struct pt_t {
//    float x, y, z;
//    pt_t() { ; }
//    pt_t(float _x, float _y, float _z) {
//        x = _x;
//        y = _y;
//        z = _z;
//    }
//};

#define RGB(r, g, b) ((int(r) << 16) + (int(g) << 8) + int(b) )

rs2::disparity_transform depth_to_disparity(true);
rs2::disparity_transform disparity_to_depth(false);

//rs2::disparity_transform depth_to_disparity(true);
//rs2::disparity_transform disparity_to_depth(false);

void ofApp::setup(){
    
    glPointSize(1.0);
    glEnable(GL_POINT_SMOOTH);
    ofSetBackgroundColor(0);
    ofSetFrameRate(30);           // using the same framerate(D435)
    ofSetVerticalSync(true);
    ofSetLogLevel(OF_LOG_NOTICE);
    ofDisableLighting();
//    ofEnableDepthTest();  // for displaying GUI  ToDo solve this
    ofEnableAlphaBlending();
    ofEnableAntiAliasing();
    mesh.setMode(OF_PRIMITIVE_POINTS);
    
    saveFlg = false; // for exporting ply file
    
    // settings for filters
    // =================================================================================
    revert_disparity = false;
    // =================================================================================
    
    ply_counter = 0;
    
    // D435 settings
    // rs-enumerate-devices for checking device informationrs-enumerate-devices
    // ToDo when hit start button, start pipe?
    // ToDo Add laser settings 12/07/2018
    // =================================================================================
    rs2::config cfg;
    int w = 1280;
    int h = 720;
//    int w = 848;
//    int h = 480;
    int frame_rate = 30;
    cfg.enable_stream(RS2_STREAM_DEPTH, w, h, RS2_FORMAT_Z16, frame_rate);
    cfg.enable_stream(RS2_STREAM_COLOR, w, h, RS2_FORMAT_RGB8, frame_rate);
    
    
    // align
    rs2_stream align_to = RS2_STREAM_COLOR;
    rs2::align align(align_to);
    
    // for controlling the laser
    selection = pipe.start(cfg);
    selected_device = selection.get_device();
    auto depth_sensor = selected_device.first<rs2::depth_sensor>();
    
    cout << "depth sensor: " << depth_sensor << endl;
    if (depth_sensor.supports(RS2_OPTION_EMITTER_ENABLED))
    {
        depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 1.f); // Enable emitter
//        depth_sensor.set_option(RS2_OPTION_EMITTER_ENABLED, 0.f); // Disable emitter
    }
    if (depth_sensor.supports(RS2_OPTION_LASER_POWER))
    {
        // Query min and max values:
        auto range = depth_sensor.get_option_range(RS2_OPTION_LASER_POWER);
//        cout << "range min: " << range.min << endl;
//        cout << "range max: " << range.max << endl;
        laser_min = range.min;
        laser_max = range.max;
        depth_sensor.set_option(RS2_OPTION_LASER_POWER, 100);
//        depth_sensor.set_option(RS2_OPTION_LASER_POWER, 0.f); // Disable laser
    }
    // =================================================================================
    
    // GUI settings
    // =================================================================================
    usedDepth.set("use depeth", false);
    scaleValue.set("scale", 200.0, 10.0, 400.0);
    alphaValue.set("alpha", 0.8, 0.1, 1.0);
    gl_point_size.set("point size", 1.0, 1.0, 5.0);
    laser_power.set("laser power", 100, laser_min, laser_max);
    
    // toggle buttons
    decimate.set("Decimate", false);
    disparity.set("Disparity", false);
    spatial.set("Spatial", false);
    temporal.set("Temporal", false);
    
    // sliders for filters
    // https://github.com/IntelRealSense/librealsense/blob/c3c758d18c585a237bb5b635927797aa69996391/doc/post-processing-filters.md
    decimate_magnitude.set("Decimate Magnitude",2 ,2 ,8);
    
    // spetial Edge-Preserving Filter
    spatial_magnitude.set("Spatial Magnitude", 2, 0, 5);
    spetial_smooth_alpha.set("Spetial Smooth Alpha", 0.50, 0.25, 1.00);
    spetial_smooth_delta.set("Spetial Smooth Delta", 20, 1, 50);
    spetial_hole_filling.set("Spetial HoleFilling", 0, 0, 5);
    
    // Temporal filter
    temporal_smooth_alpha.set("Temporal Smooth Alpha", 0.40, 0.00, 1.00);
    temporal_smooth_delta.set("Temporal Smooth Delta", 20.0, 1, 100);
    
    // Hole filter
    hole.set("Hole Filling", false);
    
    basic_parameters.setName("Basic Settings");
    basic_parameters.add(usedDepth, scaleValue, alphaValue, gl_point_size, laser_power);
    
    decimate_parameters.setName("Decimate Filter");
    decimate_parameters.add(decimate,decimate_magnitude);
    
    spetial_parameters.setName("Spetial Filter");
    spetial_parameters.add(spatial);
    spetial_parameters.add(spatial_magnitude);
    spetial_parameters.add(spetial_smooth_alpha);
    spetial_parameters.add(spetial_smooth_delta);
    spetial_parameters.add(spetial_hole_filling);
    
    temporal_parameters.setName("Temporal Filter");
    temporal_parameters.add(temporal);
    temporal_parameters.add(temporal_smooth_alpha);
    temporal_parameters.add(temporal_smooth_delta);
    
    hole_parameters.setName("Hole Filling Filter");
    hole_parameters.add(hole);
    
    
    panel = gui.addPanel(basic_parameters);
    panel->setPosition(10, 10);
    panel = gui.addPanel(decimate_parameters);
    panel->setPosition(10, 220);
    panel = gui.addPanel(spetial_parameters);
    panel->setPosition(10, 330);
    panel = gui.addPanel(temporal_parameters);
    panel->setPosition(10, 540);
    panel = gui.addPanel(hole_parameters);
    panel->setPosition(1050, 10);
    
    //    gui.add(//usedDepth, scaleValue, alphaValue, gl_point_size,
    //            decimate, disparity, spatial, temporal,
    //            decimate_magnitude,
    //            spatial_magnitude, spetial_smooth_alpha, spetial_smooth_delta,
    //            temporal_smooth_alpha, temporal_smooth_delta);
    // =================================================================================
    
    
    
}

void ofApp::update(){
    auto depth_sensor = selected_device.first<rs2::depth_sensor>();
    depth_sensor.set_option(RS2_OPTION_LASER_POWER, laser_power);
    glPointSize(gl_point_size);
    // filters
//    filters.emplace_back("Decimate", dec_filter);
//    filters.emplace_back(disparity_filter_name, depth_to_disparity);
//    filters.emplace_back("Spatial", spat_filter);
//    filters.emplace_back("Temporal", temp_filter);
    
    
    revert_disparity = disparity;  // for Disparity Filter
//    depth_to_disparity(true);
//    disparity_to_depth(false);
    

//    mesh.clearVertices();
    // Get depth data from camera
    auto frames = pipe.wait_for_frames();
    auto depth = frames.get_depth_frame();
    
    
    /* Apply filters.
     The implemented flow of the filters pipeline is in the following order:
     1. apply decimation filter
     2. transform the scence into disparity domain
     3. apply spatial filter
     4. apply temporal filter
     5. revert the results back (if step Disparity filter was applied
     to depth domain (each post processing block is optional and can be applied independantly).
     */
    
    // apply filters with default value
//    depth = (decimate)? dec_filter.process(depth) : depth;
//    depth = (spatial)? spat_filter.process(depth) : depth;
//    depth = (temporal)? temp_filter.process(depth) : depth;
    
    
    // decimation filter
    if(decimate) {
        // set value
        dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, decimate_magnitude);
        depth = dec_filter.process(depth);
    }
    
    // spatial filter
    if(spatial) {
        spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, spatial_magnitude);          // magnitude
        spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA,  spetial_smooth_alpha);   // smooth alpha
        spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, spetial_smooth_delta);    // smooth delta
        spat_filter.set_option(RS2_OPTION_HOLES_FILL, spetial_hole_filling);
        depth = spat_filter.process(depth);
    }
    
    if(temporal) {
        temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, temporal_smooth_alpha);    // smooth alpha
        temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, temporal_smooth_delta);    // smooth delta
        depth = temp_filter.process(depth);
    }
    
    revert_disparity = (disparity)? true : false;
    
    // apply Disparity
    if (revert_disparity) {
        depth = disparity_to_depth.process(depth);
    }
    
    // hole filling
//    hole_filter.set_option(RS2_OPTION_HOLES_FILL, 3);
    depth = (hole)? hole_filter.process(depth) : depth;
//    depth = hole_filter.process(depth);
    
//    depth = dec_filter.process(depth);
    // check
    /*
    for(auto &&filter : filters) {
        if(filter.is_enabled) {
            // get_depth_frame(): frame
            depth = filter.filter.process(depth);
            // check filter name
            if(filter.filter_name == disparity_filter_name) {
                revert_disparity = true;
            }
        }
    }
     */
    
    // Save the time of last frame's arrival
    auto last_time = std::chrono::high_resolution_clock::now();
    // Maximum angle for the rotation of the pointcloud
//    const double max_angle = 15.0;
    // We'll use rotation_velocity to rotate the pointcloud for a better view of the filters effects
//    float rotation_velocity = 0.3f;
    
    // Update time of current frame's arrival
//    auto curr = std::chrono::high_resolution_clock::now();
    
    // Time interval which must pass between movement of the pointcloud
//    const std::chrono::milliseconds rotation_delta(40);
    
    
    
    
    
    // for original color
    auto color = frames.get_color_frame();
    
    auto points = pc.calculate(depth);

//    if(!color)
//        color = frames.get_infrared_frame();

    pc.map_to(color);
//    points.export_to_ply("room.ply", color);
    // need to point out oF app folder
    
    
    
//    auto points = pc.calculate(depth);
    
//    app_state.tex.upload(color);
    
    
    // for post-processing
    
    
    // export to ply
    // ========================================
    if(saveFlg) {
        points.export_to_ply("/Users/koji.kanao/Documents/of_v0.10.0/apps/myApps/ofScanner/bin/data/room" + std::to_string(ply_counter) + ".ply", color);
        saveFlg = !saveFlg;
    }
    // ========================================
    
    
 
    mesh.clear();
    int n = points.size();
    const int w = color.get_width();
    const int h = color.get_height();

//    cout << w << "," << h;
        if(n!=0){
            auto vs = points.get_vertices();
            auto tex_coords = points.get_texture_coordinates();
            for(int i=0; i<n; i++){
                if(vs[i].z){
                    const rs2::vertex v = vs[i];
                    glm::vec3 v3(v.x,v.y,v.z);
                    mesh.addVertex(v3);
                    int x = std::min(std::max(int(tex_coords[i].u*w + .5f), 0), w - 1);
                    int y = std::min(std::max(int(tex_coords[i].v*h + .5f), 0), h - 1);
//                    cout << tex_coords[i].u << "," << tex_coords[i].v << endl;
                    
                    int idx = x * color.get_bytes_per_pixel() + y * color.get_stride_in_bytes();
                    const auto color_data = reinterpret_cast<const uint8_t*>(color.get_data());
                    float r = ToFloatAtCompileTime(color_data[idx]);
                    float g = ToFloatAtCompileTime(color_data[idx+1]);
                    float b = ToFloatAtCompileTime(color_data[idx+2]);
//                    cout << r << "," << g << "," << b << endl;
                    if(usedDepth) {
                        mesh.addColor(ofFloatColor(.5, .5, ofMap(v.z, 0, 10, 1, 0), 0.8));  // rgba
                    } else {
                        mesh.addColor(ofColor(r, g, b, 255 * alphaValue));
                    }
                    
                }
            }
        }
    
    // Create oF mesh
//    mesh.clear();
//    int n = points.size();
//    if(n!=0){
//        const rs2::vertex * vs = points.get_vertices();
//        for(int i=0; i<n; i++){
//            if(vs[i].z){
//                const rs2::vertex v = vs[i];
//                glm::vec3 v3(v.x,v.y,v.z);
//                mesh.addVertex(v3);
//

//            }
//        }
//    }
    if(saveFlg) {
        cout << "export to ply" << endl;
//        points.export_to_ply("room.ply", color);
//        points.export_to_ply("/Users/koji.kanao/Documents/of_v0.10.0/apps/myApps/ofScanner/bin/data/room.ply", color);
    }
}

void ofApp::draw(){
    
    ofBackground(200);
    
    cam.begin();
    ofScale(scaleValue, -scaleValue, -scaleValue);
//    ofDrawAxis(1);
    
    ofPushMatrix();
    ofTranslate(0, 1, 0);
    ofRotateZDeg(90);
    ofSetColor(255);
    ofDrawGridPlane(1, 10, true);
    ofPopMatrix();
    mesh.draw();
    cam.end();

}



void ofApp::keyPressed(int key){
    if(key == 's'){
        ply_counter++;
        cout << ply_counter << " pressed s" << endl;
        saveFlg = !saveFlg;
        //mesh.save("test.ply");
//        exportPlyCloud(mesh, "model.ply");
        // export pcd
        //***********************************************************
//        FILE *fp = fopen("./test.pcd", "wb");
//        fprintf(fp, "# .PCD v.7 - Point Cloud Data file format\n");
//        fprintf(fp, "VERSION .7\nFIELDS x y z rgb\nSIZE 4 4 4 4\nTYPE F F F U\nCOUNT 1 1 1 1\n");
//        fprintf(fp, "WIDTH %lu\nHEIGHT 1\nVIEWPOINT 0 0 0 1 0 0 0\nPOINTS %lu\nDATA binary\n", points.size(), points.size());
//        // binary
//        for (int i = 0; i < points.size(); i++) {
//            // colored
//            unsigned int col = RGB(points.get_vertices()[i].z * 5, 128 + 3*points.get_vertices()[i].x, 128 + 3 * points.get_vertices()[i].y);
//            float xyz[3] = { points.get_vertices()[i].x, points.get_vertices()[i].y, points.get_vertices()[i].z};
//            fwrite( xyz, sizeof(float), 3, fp);
//            fwrite(&col, sizeof(unsigned int), 1, fp);
//        }
//        fclose(fp);
        // export pcd
        //***********************************************************
    }
}


void ofApp::exportPlyCloud(ofMesh pcMesh, string filename){
    
    ofFile ply;
    if(ply.open(filename, ofFile::WriteOnly)){
        // write the header
        ply << "ply" << endl;
        ply << "format binary_little_endian 1.0" << endl;
        ply << "element vertex " << pcMesh.getVertices().size() << endl;
        ply << "property float x" << endl;
        ply << "property float y" << endl;
        ply << "property float z" << endl;
        ply << "end_header" << endl;
        
        // write file
        vector<glm::vec3>&surface = pcMesh.getVertices();
        for(int i = 0; i < surface.size(); i++){
            if(surface[i].z != 0){
                // write the raw data as if it were a stream of bytes
                ply.write((char *) &surface[i], sizeof(ofVec3f));
            }
        }
    }
}

