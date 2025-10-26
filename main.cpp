#include <opencv2/opencv.hpp>
#include <iostream>
#include <sys/ioctl.h>
#include <unistd.h>
#include <algorithm>
#include <cstring>

using namespace cv;
using namespace std;

// Convert cv::Mat to raw RGB data
unsigned char* matToRawData(const cv::Mat& frame, int& width, int& height, int& channels) {
    Mat rgb;
    if (frame.channels() == 1) {
        cvtColor(frame, rgb, COLOR_GRAY2RGB);
    } else if (frame.channels() == 4) {
        cvtColor(frame, rgb, COLOR_BGRA2RGB);
    } else {
        cvtColor(frame, rgb, COLOR_BGR2RGB);
    }

    width = rgb.cols;
    height = rgb.rows;
    channels = 3;

    unsigned char* data = new unsigned char[width * height * channels];
    memcpy(data, rgb.data, width * height * channels);

    return data;
}

// Downsample image by scale factor
unsigned char* downsampleImage(unsigned char* originalData, int* width, int* height, int* channels, int scale) {
    int scaledX = scale;
    int scaledY = scale * 2;

    int newWidth  = *width / scaledX;
    int newHeight = *height / scaledY;

    unsigned char* newData = new unsigned char[newWidth * newHeight * (*channels)];

    for (int y = 0; y < newHeight; y++) {
        for (int x = 0; x < newWidth; x++) {
            int sumR = 0, sumG = 0, sumB = 0, count = 0;

            for (int dy = 0; dy < scaledY; dy++) {
                for (int dx = 0; dx < scaledX; dx++) {
                    int origX = x * scaledX + dx;
                    int origY = y * scaledY + dy;

                    if (origX >= *width || origY >= *height) continue;

                    int index = (origY * *width + origX) * *channels;
                    sumR += originalData[index + 0];
                    sumG += originalData[index + 1];
                    sumB += originalData[index + 2];
                    count++;
                }
            }

            int newIndex = (y * newWidth + x) * *channels;
            newData[newIndex + 0] = sumR / count;
            newData[newIndex + 1] = sumG / count;
            newData[newIndex + 2] = sumB / count;
        }
    }

    *width = newWidth;
    *height = newHeight;
    return newData;
}

// Print ASCII from raw image data
void createImageFromFrame(unsigned char* data, int width, int height, int channels, int scale = 1, bool color = false) {
    string ascii = " .:-=+*#%@";

    data = downsampleImage(data, &width, &height, &channels, scale);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int index = (y * width + x) * channels;
            int R = data[index + 0];
            int G = data[index + 1];
            int B = data[index + 2];

            int brightness = 0.2126 * R + 0.7152 * G + 0.0722 * B;
            int c = (brightness * (ascii.length() - 1)) / 255;

            if (!color) {
                cout << ascii[c];
            } else {
                string colorCode = "\033[38;2;" + to_string(R) + ";" + to_string(G) + ";" + to_string(B) + "m";
                cout << colorCode << ascii[c] << "\033[0m";
            }
        }
        cout << endl;
    }

    delete[] data;  // free memory
}

// Move cursor to top-left (no flashing)
void moveCursorTopLeft() {
    cout << "\033[H" << flush;
}

// Get terminal size
void getTerminalSize(int& cols, int& rows) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    cols = w.ws_col;
    rows = w.ws_row;
}

int main() {
    cout << std::unitbuf;

    VideoCapture cap(0);
    if (!cap.isOpened()) {
        cerr << "Error: Could not open webcam." << endl;
        return -1;
    }

    cout << std::unitbuf;
    Mat frame;
    while (true) {
        cap.read(frame);
        if (frame.empty()) break;

        int width, height, channels;
        unsigned char* data = matToRawData(frame, width, height, channels);

        int termCols, termRows;
        getTerminalSize(termCols, termRows);

        float charAspect = 0.5f;
        float scaleX = float(width) / termCols;
        float scaleY = float(height) / (termRows / charAspect);
        int finalScale = max(1, int(max(scaleX, scaleY))) + 2;

        moveCursorTopLeft();
        createImageFromFrame(data, width, height, channels, finalScale, false);

        delete[] data;
    }

    cap.release();
    destroyAllWindows();
    return 0;
}
