#include <XPLMDisplay.h>
#include <XPLMGraphics.h>
#include <XPLMProcessing.h>
#include <XPLMUtilities.h>
#include <XPLMDataAccess.h>
#include <string.h>
#include <iostream>
#include <thread>
#include <atomic>
#include <arpa/inet.h>
#include <unistd.h>
#if IBM
	#include <windows.h>
#endif
#if LIN
	#include <GL/gl.h>
#elif __GNUC__
	#include <OpenGL/gl.h>
#else
	#include <GL/gl.h>
#endif

#ifndef XPLM300
	#error This is made to be compiled against the XPLM300 SDK
#endif

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

static XPLMWindowID g_window;
static char g_display_data[BUFFER_SIZE] = "Waiting for data...";
static int g_sock = 0;
static std::atomic<bool> g_running(true);
static std::thread g_thread;
static int g_toggle_strobe = 0;

void TCPClientThread() {
    char buffer[BUFFER_SIZE] = {0};
    while (g_running.load()) {
        int valread = read(g_sock, buffer, BUFFER_SIZE);
        if (valread > 0) {
            if (buffer[0] == 'A') {
                XPLMDataRef icao_ref = XPLMFindDataRef("sim/aircraft/view/acf_ICAO");
                XPLMGetDatab(icao_ref, g_display_data, 0, sizeof(g_display_data));
            } else if (buffer[0] == 'B') {
                XPLMDataRef strobe_ref = XPLMFindDataRef("sim/cockpit/electrical/strobe_lights_on");
                g_toggle_strobe = !XPLMGetDatai(strobe_ref);
                XPLMSetDatai(strobe_ref, g_toggle_strobe);
            }
        }
    }
}

void MyDrawWindowCallback(XPLMWindowID in_window_id, void *in_refcon) {
    int left, top, right, bottom;
    XPLMGetWindowGeometry(in_window_id, &left, &top, &right, &bottom);

    XPLMDrawString(XPLM_COLOR_WHITE, left + 10, top - 20, g_display_data, NULL, xplmFont_Basic);
}

void MyHandleMouseClickCallback(XPLMWindowID in_window_id, int x, int y, XPLMMouseStatus in_mouse, void *in_refcon) {
    // Do nothing for now
}

PLUGIN_API int XPluginStart(char *outName, char *outSig, char *outDesc) {
    strcpy(outName, "TCPClientPlugin");
    strcpy(outSig, "xpsdk.examples.tcpclient");
    strcpy(outDesc, "A plugin that acts as a TCP client and performs actions based on received data.");

    // Create the window
    int left = 100;
    int top = 600;
    int right = 600;
    int bottom = 500;
    g_window = XPLMCreateWindow(left, top, right, bottom, 1, MyDrawWindowCallback, MyHandleMouseClickCallback, NULL, NULL);

    // Setup TCP client
    struct sockaddr_in serv_addr;
    if ((g_sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation error");
        return 0;
    }
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);

    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        perror("Invalid address/Address not supported");
        return 0;
    }

    if (connect(g_sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Connection Failed");
        return 0;
    }

    // Create a thread to handle the TCP client
    g_thread = std::thread(TCPClientThread);

    return 1;
}

PLUGIN_API void XPluginStop(void) {
    g_running.store(false);
    if (g_thread.joinable()) {
        g_thread.join();
    }
    close(g_sock);
    XPLMDestroyWindow(g_window);
}

PLUGIN_API void XPluginDisable(void) {}

PLUGIN_API int XPluginEnable(void) { return 1; }

PLUGIN_API void XPluginReceiveMessage(XPLMPluginID inFromWho, int inMessage, void *inParam) {}
