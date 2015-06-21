#include "testApp.h"


//--------------------------------------------------------------
//----------------------  Params -------------------------------
//--------------------------------------------------------------
Params param;        //Definition of global variable

void Params::setup() {
    eCenter = ofPoint( ofGetWidth() / 2, ofGetHeight() / 2 );
    eRad = 100;
    velRad = 40;
    lifeTime = 2.0;
    
    rotate = -140;
    force = -540;
    spinning = 280;
    friction = 0.05;
}

//--------------------------------------------------------------
//----------------------  Particle  ----------------------------
//--------------------------------------------------------------
Particle::Particle() {
    live = false;
}

//--------------------------------------------------------------
ofPoint randomPointInCircle( float maxRad ){
    ofPoint pnt;
    float rad = ofRandom( 0, maxRad );
    float angle = ofRandom( 0, M_TWO_PI * 4 );
    pnt.x = sin( angle+PI/3 ) * rad * 10;
    pnt.y = cos( angle + PI/3 ) * rad * 10;
    return pnt;
}

//--------------------------------------------------------------
void Particle::setup() {
    pos = param.eCenter + randomPointInCircle( param.eRad );
    vel = randomPointInCircle( param.velRad );
    time = 0;
    lifeTime = param.lifeTime;
    live = true;
}

//--------------------------------------------------------------
void Particle::update( float dt ){
    if ( live ) {
        //Rotate vel
        vel.rotate( 0, 0, param.rotate * dt );
        
        ofPoint acc;         //Acceleration
        ofPoint delta = pos - param.eCenter;
        float len = delta.length();
        if ( ofInRange( len, 0, param.eRad ) ) {
            delta.normalize();
            
            //Attraction/repulsion force
            acc += delta * param.force;
            
            //Spinning force
            acc.x += -delta.y * param.spinning;
            acc.y += delta.x * param.spinning;
        }
        vel += acc * dt;            //Euler method
        vel *= ( 1 - param.friction );  //Friction
        
        //Update pos
        pos += vel * dt;    //Euler method
        
        //Update time and check if particle should die
        time += dt;
        if ( time >= lifeTime ) {
            live = false;   //Particle is now considered as died
        }
    }
}

//--------------------------------------------------------------
void Particle::draw( float a ){
    if ( live ) {
        //Compute size
        //put the audio input in as "a"
        float size = ofMap(
                           fabs(time - lifeTime/2), 0, lifeTime/2, 3, 4*a );
        
        //Compute color
        ofColor color = ofColor::red;
        float hue = ofMap( time, 0, lifeTime, 0, 255 );
        color.setHue( hue );
        ofSetColor( color );
        
//        ofCircle( pos, size );  //Draw particle
        ofRect(pos, size*2, size*2);
//        ofDrawIcoSphere(pos, size);
//        ofDrawPlane(pos, size, size);
        
    }
}

//--------------------------------------------------------------
//----------------------  testApp  -----------------------------
//--------------------------------------------------------------
void testApp::setup(){
    //audio stuff
    //***********************************************
    //***********************************************
    // listen on the given port
    cout << "listening for osc messages on port " << PORT << "\n";
    receiver.setup(PORT);
    
    current_msg_string = 0;
    mouseX = 0;
    mouseY = 0;
    mouseButtonState = "";
    
//    ofBackground(30, 30, 130);
    //i reset the bg color, but not works
    ofBackground(0, 0, 0);
    
    nBandsToUse = 1024;
    receivedFft = new float[nBandsToUse];
    for (int i = 0; i < nBandsToUse; i++){
        receivedFft[i] = 0;
    }
    //***********************************************
    //***********************************************
    
    
    
    ofSetFrameRate( 60 );	//Set screen frame rate
    
    //Allocate drawing buffer
    int w = ofGetWidth();
    int h = ofGetHeight();
    fbo.allocate( w, h, GL_RGB32F_ARB );
    
    //Fill buffer with white color
    fbo.begin();
    ofBackground(0, 0, 0);
    fbo.end();
    
    //Set up parameters
    param.setup();		//Global parameters
    history = 0.25;
    bornRate = 20;
    
    bornCount = 0;
    time0 = ofGetElapsedTimef();
    
    //GUI
    interf.setup();
    interf.addSlider( "rate", &bornRate, 0, 3000 );
    interf.addSlider( "lifeTime", &param.lifeTime, 0, 5 );
    interf.addSlider( "history", &history, 0, 1 );
    
    interf.addSlider( "eRad", &param.eRad, 0, 800 );
    interf.addSlider( "velRad", &param.velRad, 0, 400 );
    interf.addSlider( "rotate", &param.rotate, -500, 500 );
    interf.addSlider( "spinning", &param.spinning, -1000, 1000 );
    interf.addSlider( "force", &param.force, -1000, 1000 );
    interf.addSlider( "friction", &param.friction, 0, 0.1 );
    
    drawInterface = true;
}

//--------------------------------------------------------------
void testApp::update(){
    
    //audio stuff
    //***********************************************
    //***********************************************
    // hide old messages
    for(int i = 0; i < NUM_MSG_STRINGS; i++){
        if(timers[i] < ofGetElapsedTimef()){
            msg_strings[i] = "";
        }
    }
    
    // check for waiting messages
    while(receiver.hasWaitingMessages()){
        // get the next message
        ofxOscMessage m;
        receiver.getNextMessage(&m);
        
        if(m.getAddress() == "/channel_01/FFT"){
            int index = m.getArgAsInt32(0);
            float val = m.getArgAsFloat(1);
            receivedFft[index] = val;
            //cout << "recvd: " << index << " :\t"<<val<<endl;
        }
        
        else {
            string msg_string;
            msg_string = m.getAddress();
            cout << "unrecognized message" << msg_string << endl;
        }
    }
    
    keyValue = receivedFft[50]*5;
    keyValue = ofMap(keyValue, 0.0, 1.0, 0.0, 8.0);
    keyValue *= 0.95;
    //***********************************************
    //***********************************************
    
    
    //Compute dt
    float time = ofGetElapsedTimef();
    float dt = ofClamp( time - time0, 0, 0.1*keyValue*10 );
    time0 = time;
    
    //Delete inactive particles
    int i=0;
    while (i < p.size()) {
        if ( !p[i].live ) {
            p.erase( p.begin() + i );
        }
        else {
            i++;
        }
    }
    
    //Born new particles
    bornCount += dt * bornRate;      //Update bornCount value
    if ( bornCount >= 1 ) {          //It's time to born particle(s)
        int bornN = int( bornCount );//How many born
        bornCount -= bornN;          //Correct bornCount value
        for (int i=0; i<bornN; i++) {
            Particle newP;
            newP.setup();            //Start a new particle
            p.push_back( newP );     //Add this particle to array
        }
    }
    
    //Update the particles
    for (int i=0; i<p.size(); i++) {
        p[i].update( dt );
    }
}

//--------------------------------------------------------------
void testApp::draw(){
    
    //audio stuff
    //***********************************************
    //***********************************************
    ofEnableAlphaBlending();
    ofSetColor(255,255,255,100);
    ofRect(25,25,ofGetWidth()-50,225);
    ofDisableAlphaBlending();
    
    // draw the fft resutls:
    ofSetColor(255,255,255,255);
    
    float width = 3;
    for (int i = 0;i < nBandsToUse; i++){
        //cout << i << "\t"<<fftSmoothed[i] << endl;
        ofRect(25+i*width,250,width,-(receivedFft[i] * 750));
        if(i%10 == 0){
            ofDrawBitmapString(ofToString(i), 25+i*width, 265);
        }
    }
    //    ofDrawBitmapString("listening on PORT "+ofToString(PORT), 25, ofGetHeight()-80);
    
    ofDrawBitmapString("THIS IS YOUR APP, DO WHATEVER YOU WANT WITH THIS DATA!!"+ofToString(PORT), 25, ofGetHeight()-50);
    //***********************************************
    //***********************************************
    
    
    ofBackground( 255, 255, 255 );  //Set white background
    
    //1. Drawing to buffer
    fbo.begin();
    
    //Draw semi-transparent white rectangle
    //to slightly clearing a buffer (depends on history value)
    
    ofEnableAlphaBlending();         //Enable transparency
    
    // the bg color is here!!!!!!!!!!!!!!!!!!!!!!!!!!
    //bgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbg
    //bgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbgbg
    float alpha = (1-history) * 255;
//    ofBackgroundGradient( ofColor( 20 ), ofColor( 10 ),alpha );
    ofSetColor( 20, 20, 20, alpha );
    ofFill();
    ofRect( 0, 0, ofGetWidth(), ofGetHeight() );
    
    ofDisableAlphaBlending();        //Disable transparency
    
    //Draw the particles
    ofFill();
    for (int i=0; i<p.size(); i++) {
        p[i].draw(keyValue);
    }
    
    fbo.end();
    
    //2. Draw buffer on the screen
    ofSetColor( 255, 255, 255 );
    fbo.draw( 0, 0 );
    
    //GUI
    if ( drawInterface ) {
        //Draw text
        ofSetColor( 0, 0, 0 );
        ofDrawBitmapString( "Keys: Enter-hide/show GUI, Space-screenshot, 1,2,...,9-load preset, Shift+1,2,...9-save preset", 20, 20 );
        ofDrawBitmapString( "Particles: " + ofToString( p.size() ), 20, 40 );
        
        //Draw sliders
        interf.draw();
        
        //Draw emitter as a circle
        ofSetCircleResolution( 50 );
        ofNoFill();
        ofSetColor( 128, 128, 128 );
        ofCircle( param.eCenter, param.eRad );
        ofSetCircleResolution( 20 );
    }
}

//--------------------------------------------------------------
//----------------------  GUI ----------------------------------
//--------------------------------------------------------------
void Interface::setup(){
    selected = -1;
}

void Interface::addSlider( string title, float *value, float minV, float maxV ){
    Slider s;
    s.title = title;
    s.rect = ofRectangle( 20, 60 + slider.size() * 40, 150, 30 );
    s.value = value;
    s.minV = minV;
    s.maxV = maxV;
    slider.push_back( s );
}

void Interface::draw(){
    for (int i=0; i<slider.size(); i++) {
        Slider &s = slider[i];
        ofRectangle r = s.rect;
        ofFill();
        ofSetColor( 255, 255, 255 );
        ofRect( r );
        ofSetColor( 0, 0, 0 );
        ofNoFill();
        ofRect( r );
        ofFill();
        float w = ofMap( *s.value, s.minV, s.maxV, 0, r.width );
        ofRect( r.x, r.y + 15, w, r.height - 15 );
        ofDrawBitmapString( s.title + " " + ofToString( *s.value, 2 ), r.x + 5, r.y + 12 );
    }
}

void Interface::mousePressed( int x, int y ){
    for (int i=0; i<slider.size(); i++) {
        Slider &s = slider[i];
        ofRectangle r = s.rect;
        if ( ofInRange( x, r.x, r.x + r.width ) && ofInRange( y, r.y, r.y + r.height ) ) {
            selected = i;
            *s.value = ofMap( x, r.x, r.x + r.width, s.minV, s.maxV, true );
        }
    }
}

void Interface::mouseDragged( int x, int y ){
    if ( selected >= 0 ) {
        Slider &s = slider[selected];
        ofRectangle r = s.rect;
        *s.value = ofMap( x, r.x, r.x + r.width, s.minV, s.maxV, true );
    }
}

void Interface::mouseReleased (int x, int y ){
    selected = -1;
}

//--------------------------------------------------------------
//For saving/loading presets we use very simple method:
//just pack all sliders values into list and save/load it from file.
//Its very simple, but is not practical, because when saved,
//you can not change the number of sliders and these order.
//The best solution is using ofxXmlSettings - it lets write sliders values
//and specify these names

void Interface::save( int index )
{
    vector<string> list;
    for (int i=0; i<slider.size(); i++) {
        list.push_back( ofToString( *slider[i].value ) );
    }
    string text = ofJoinString( list," " );
    string fileName = "preset" + ofToString( index ) + ".txt";
    ofBuffer buffer = ofBuffer( text );
    ofBufferToFile( fileName, buffer );
}

//--------------------------------------------------------------
void Interface::load( int index )
{
    string fileName = "preset" + ofToString( index ) + ".txt";
    string text = string( ofBufferFromFile( fileName ) );
    vector<string> list = ofSplitString( text, " " );
    
    if ( list.size() == slider.size() ) {
        for (int i=0; i<slider.size(); i++) {
            *slider[i].value = ofToFloat( list[i] );
        }
    }
}

//--------------------------------------------------------------
//----------------------  testApp again  -----------------------
//--------------------------------------------------------------
void testApp::keyPressed(int key){
    if ( key == OF_KEY_RETURN ) {	//Hide/show GUI
        drawInterface = !drawInterface;
    }
    
    if ( key == ' ' ) {		//Grab the screen image to file
        ofImage image;
        image.grabScreen( 0, 0, ofGetWidth(), ofGetHeight() );
        
        //Select random file name
        int n = ofRandom( 0, 1000 );
        string fileName = "screen" + ofToString( n ) + ".png";
        
        image.saveImage( fileName );
    }
    
    //Load presets
    if ( key == '1' ) { interf.load( 1 ); }
    if ( key == '2' ) { interf.load( 2 ); }
    if ( key == '3' ) { interf.load( 3 ); }
    if ( key == '4' ) { interf.load( 4 ); }
    if ( key == '5' ) { interf.load( 5 ); }
    if ( key == '6' ) { interf.load( 6 ); }
    if ( key == '7' ) { interf.load( 7 ); }
    if ( key == '8' ) { interf.load( 8 ); }
    if ( key == '9' ) { interf.load( 9 ); }
    
    //Save presets
    if ( key == '!' ) { interf.save( 1 ); }
    if ( key == '@' ) { interf.save( 2 ); }
    if ( key == '#' ) { interf.save( 3 ); }
    if ( key == '$' ) { interf.save( 4 ); }
    if ( key == '%' ) { interf.save( 5 ); }
    if ( key == '^' ) { interf.save( 6 ); }
    if ( key == '&' ) { interf.save( 7 ); }
    if ( key == '*' ) { interf.save( 8 ); }
    if ( key == '(' ) { interf.save( 9 ); }
}

//--------------------------------------------------------------
void testApp::keyReleased(int key){
    
}

//--------------------------------------------------------------
void testApp::mouseMoved(int x, int y){
    
}

//--------------------------------------------------------------
void testApp::mouseDragged(int x, int y, int button){
    if ( drawInterface ) {
        interf.mouseDragged( x, y );
    }
}

//--------------------------------------------------------------
void testApp::mousePressed(int x, int y, int button){
    if ( drawInterface ) {
        interf.mousePressed( x, y );
    }
}

//--------------------------------------------------------------
void testApp::mouseReleased(int x, int y, int button){
    interf.mouseReleased( x, y );
}

//--------------------------------------------------------------
void testApp::windowResized(int w, int h){
    
}

//--------------------------------------------------------------
void testApp::gotMessage(ofMessage msg){
    
}

//--------------------------------------------------------------
void testApp::dragEvent(ofDragInfo dragInfo){
    
}