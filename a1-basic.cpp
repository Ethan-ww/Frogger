#include <iostream>
#include <list>
#include <cstdlib>
#include <unistd.h>
#include <vector>
#include <string>
#include <sstream>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <sys/time.h>

using namespace std;


const int Border = 5;
const int BufferSize = 10;

int level=1;
int FPS=30;

int frog_x=400;
int frog_y=200;


void reset_floor(int l=1){
    frog_x=400;
    frog_y=200;
    level=l;
}


bool belong_to(int x, int left_bound, int right_bound){
    return x<=right_bound && x>=left_bound;
}

struct XInfo {
    Display*  display;
    Window   window;
    GC       gc;
};

unsigned long now(){
    timeval tv;
    gettimeofday(&tv, NULL);
    return tv.tv_sec* 1000000 + tv.tv_usec;
}


// An abstract class representing displayable things.
class Displayable {
public:
    virtual void paint(XInfo& xinfo) = 0;
    virtual void move(string direction="") = 0;
};


// A displayable block
class Block : public Displayable {
public:
    virtual void paint(XInfo& xinfo) {
        XFillRectangle(xinfo.display, xinfo.window, xinfo.gc, x, y, width, height);
        if(direction){ // move left
            if(x<0) XFillRectangle(xinfo.display, xinfo.window, xinfo.gc, 850+x, y, width, height);
        }else{ // move right
            if(x+width>850) XFillRectangle(xinfo.display, xinfo.window, xinfo.gc, 0, y, x+width-850, height);
        }
    }
    
    Block(int x, int y, int width, int height, bool direction) {
        this->x=x;
        this->y=y;
        this->width=width;
        this->height=height;
        this->direction=direction;
    }
    
    virtual void move(string str=""){
        if(direction){
            // collision judgement
            
            if(frog_y==y){ // same y_cord
                if(x>=0){
                    if(belong_to(frog_x, x, x+width) ||
                       belong_to(frog_x+50, x, x+width)){
                        reset_floor();
                    }
                }else{
                    if(belong_to(frog_x, x, x+width) ||
                       belong_to(frog_x+50, x, x+width) ||
                       belong_to(frog_x, -x, -x+width) ||
                       belong_to(frog_x+50, -x, -x+width)){
                        reset_floor();
                    }
                }
            }
            x-=level;
            if(x<-width) x=850-width;
        }else{
            if(frog_y==y){ // same y_cord
                if(x<=850-width){
                    if(belong_to(frog_x, x, x+width) ||
                       belong_to(frog_x+50, x, x+width)){
                        reset_floor();
                    }
                }else{
                    if(belong_to(frog_x, x, x+width) ||
                       belong_to(frog_x+50, x, x+width) ||
                       belong_to(frog_x, 0, x+width-850) ||
                       belong_to(frog_x+50, 0, x+width-850)){
                        reset_floor();
                    }
                }
            }
            
            x+=level;
            if(x>850) x=0;
        }
    }
    
private:
    int x, y, width, height;
    // ture: left
    // false: right
    bool direction;
};

// The displayable frog
class Frog : public Displayable{
public:
    virtual void paint(XInfo& xinfo){
        XFillRectangle(xinfo.display, xinfo.window, xinfo.gc, x, y, width, height);
    }
    
    Frog(int &x, int &y):x{x},y{y}{}
    
    virtual void move(string direction=""){
        if(direction=="up"){
            if(y>=50) y-=50;
        }else if(direction=="down"){
            if(y<=150) y+=50;
        }else if(direction=="left"){
            if(x>=50) x-=50;
        }else if(direction=="right"){
            if(x<=750) x+=50;
        }else{
            // do nothing
        }
    }
    
private:
    int &x,&y;
    int width=50,height=50;
};

// Function to put out a message on error exits.
void error( string str ) {
    cerr << str << endl;
    exit(0);
}

// Function to repaint a display list
void repaint( list<Displayable*> dList, XInfo& xinfo) {
    list<Displayable*>::const_iterator begin = dList.begin();
    list<Displayable*>::const_iterator end = dList.end();
    
    XClearWindow( xinfo.display, xinfo.window );
    
    // display the level info
    ostringstream oss;
    oss<<"Level: "<<level;
    string str=oss.str();
    XDrawImageString( xinfo.display, xinfo.window, xinfo.gc,
                     700, 20, str.c_str(), str.length());
    while ( begin != end ) {
        Displayable* d = *begin;
        d->paint(xinfo);
        begin++;
    }
    XFlush( xinfo.display );
}


// The loop responding to events from the user.
void eventloop(XInfo& xinfo) {
    XEvent event;
    KeySym key;
    char text[BufferSize];
    list<Displayable*> dList;
    // dList table:
    // 0: Frog
    // 1,2,3: blocks at layer 1
    // 4,5,6,7: blocks at layer 2
    // 8,9: blocks at layer 3
    dList.push_back(new Frog(frog_x,frog_y));
    
    
    dList.push_back(new Block(0,50,50,50,false));
    dList.push_back(new Block(283,50,50,50,false));
    dList.push_back(new Block(566,50,50,50,false));
    
    dList.push_back(new Block(0,100,20,50,true));
    dList.push_back(new Block(212,100,20,50,true));
    dList.push_back(new Block(425,100,20,50,true));
    dList.push_back(new Block(637,100,20,50,true));
    
    dList.push_back(new Block(0,150,100,50,false));
    dList.push_back(new Block(425,150,100,50,false));
    
    string str;
    unsigned long lastRepaint=now();
    
    while ( true ) {
        if(XPending(xinfo.display)>0){
            XNextEvent( xinfo.display, &event );
            switch ( event.type ) {
                case KeyPress:
                    int i = XLookupString((XKeyEvent*)&event, text, BufferSize, &key, 0 );
                    // Note:
                    // left: 65361
                    // up: 65362
                    // right: 65363
                    // down: 65364
                    
                    list<Displayable*>::const_iterator begin = dList.begin();
                    Displayable* d = *begin;
                    switch (key) {
                        case 65361:
                            d->move("left");
                            break;
                            
                        case 65362:
                            d->move("up");
                            break;
                            
                        case 65363:
                            d->move("right");
                            break;
                            
                        case 65364:
                            d->move("down");
                            break;
                            
                        default:
                            break;
                    }
                    
                    // deal with collision here
                    
                    if ( i == 1 && text[0] == 'n' && frog_y==0) {
                        level++;
                        reset_floor(level);
                        // add reset here
                    }
                    
                    if ( i == 1 && text[0] == 'q' ) { // exit
                        cout << "Terminated normally." << endl;
                        XCloseDisplay(xinfo.display);
                        return;
                    }
                    break;
            }
            
        }
        
        unsigned long end=now();
        
        if(end-lastRepaint>1000000/FPS){
            list<Displayable*>::const_iterator begin = dList.begin();
            list<Displayable*>::const_iterator end = dList.end();
            while ( begin != end ) {
                Displayable* d = *begin;
                d->move();
                begin++;
            }
            repaint(dList, xinfo);
            lastRepaint=now();
        }
        
        if (XPending(xinfo.display) == 0) {
            usleep(1000000 / FPS -(end -lastRepaint));
        }
        
    }
}

//  Create the window;  initialize X.
void initX(int argc, char* argv[], XInfo& xinfo) {
    if(argc==2){
        string str=argv[1];
        istringstream iss(str);
        iss>>FPS;
    }
    
    xinfo.display = XOpenDisplay( "" );
    if ( !xinfo.display ) {
        error( "Can't open display." );
    }
    
    int screen = DefaultScreen( xinfo.display );
    unsigned long background = WhitePixel( xinfo.display, screen );
    unsigned long foreground = BlackPixel( xinfo.display, screen );
    
    
    XSizeHints hints;
    hints.x = 100;
    hints.y = 100;
    hints.width = 850;
    hints.height = 250;
    hints.flags = PPosition | PSize;
    xinfo.window = XCreateSimpleWindow( xinfo.display, DefaultRootWindow( xinfo.display ),
                                       hints.x, hints.y, hints.width, hints.height,
                                       Border, foreground, background );
    XSetStandardProperties( xinfo.display, xinfo.window, "Frogger-basic", "Frogger-basic", None,
                           argv, argc, &hints );
    
    
    xinfo.gc = XCreateGC (xinfo.display, xinfo.window, 0, 0 );
    
    // load a larger font
    XFontStruct * font;
    font = XLoadQueryFont (xinfo.display, "12x24");
    XSetFont (xinfo.display, xinfo.gc, font->fid);
    
    XSetBackground( xinfo.display, xinfo.gc, background );
    XSetForeground( xinfo.display, xinfo.gc, foreground );
    
    // Tell the window manager what input events you want.
    XSelectInput( xinfo.display, xinfo.window, KeyPressMask);
    
    XMapRaised( xinfo.display, xinfo.window );
}



int main ( int argc, char* argv[] ) {
    XInfo xinfo;
    initX(argc, argv, xinfo);
    eventloop(xinfo);
}


