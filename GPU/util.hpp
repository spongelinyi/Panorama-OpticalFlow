#ifndef util_hpp
#define util_hpp

#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <iostream>
#include <limits>
#include <vector>
#include <assert.h>
#include <execinfo.h>
#include <signal.h>
#include <exception>
#include <stdexcept>
#include <chrono>

#include <opencv2/opencv.hpp>
#include "opencv2/calib3d.hpp"
#include "opencv2/core.hpp"
#include "opencv2/core/utility.hpp"
#include "opencv2/highgui.hpp"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"

#include <gflags/gflags.h>
#include <glog/logging.h>
#ifndef CV_BGRA2GRAY
#define CV_BGRA2GRAY cv::COLOR_BGRA2GRAY
#endif
#ifndef CV_INTER_CUBIC
#define CV_INTER_CUBIC cv::INTER_CUBIC
#endif
#ifndef CV_INTER_LINEAR
#define CV_INTER_LINEAR cv::INTER_LINEAR
#endif
#ifndef CV_GRAY2BGRA
#define CV_GRAY2BGRA cv::COLOR_GRAY2BGRA
#endif
#ifndef CV_BGR2BGRA
#define CV_BGR2BGRA cv::COLOR_BGR2BGRA
#endif
#ifndef CV_HSV2BGR
#define CV_HSV2BGR cv::COLOR_HSV2BGR
#endif
#ifndef CV_AA
#define CV_AA cv::LINE_AA
#endif
#ifndef CV_THRESH_BINARY
#define CV_THRESH_BINARY cv::THRESH_BINARY
#endif
namespace fLB {
    extern bool FLAGS_help;
    extern bool FLAGS_helpshort;
}

namespace util {
    using namespace std;
    using namespace cv;
    using namespace chrono;
    
    struct VrCamException : public std::exception {
        std::string msg;
        VrCamException() {}
        VrCamException(const std::string& msg) : msg(msg) {}
        const char* what() const noexcept { return msg.c_str(); }
    };
    
    static void requireArg(const string& argValue, const string& argName) {
        if (argValue.empty()) {
            throw VrCamException("missing required command line argument: " + argName);
        }
    }
    
    static double getCurrTimeSec() {
        return (double)(system_clock::now().time_since_epoch().count()) * system_clock::period::num / system_clock::period::den;
    }
    
    //load image
    Mat imreadExceptionOnFail(const string& filename, const int flags);
    
    //write image
    void imwriteExceptionOnFail(
                                const string& filename,
                                const Mat& image,
                                const vector<int>& params = vector<int>());
    
    Mat stackHorizontal(const std::vector<Mat>& images);
    
    void printStacktrace();
    
    void terminateHandler();
    
    void sigHandler(int signal);
    
    void initOpticalFlow(int argc, char** argv);
    
    // <--------------------------------------------------------------------------------->
    template <typename T>
    inline T square(const T x) { return x * x; }
    
    template <typename T>
    inline T clamp(const T& x, const T& a, const T& b) {
        return x < a ? a : x > b ? b : x;
    }
    
    template <typename T>
    inline T reflect(const T x, const T r) {
        return x < T(0) ? -x : x >= r ? 2*r - x - 2 : x;
    }
    
    template <typename T>
    inline T wrap(const T x, const T r) {
        return x < T(0) ? r + x : x >= r ? x - r : x;
    }
    
    template <typename V, typename T>
    inline V lerp(const V x0, const V x1, const T alpha) {
        return x0 * (T(1) - alpha) + x1 * alpha;
    }
    
    template <typename T>
    inline T lerp(const T x0, const T x1, const T alpha) {
        return x0 * (T(1) - alpha) + x1 * alpha;
    }
    
    template <typename T>
    inline T bilerp(
                    const T x00,
                    const T x01,
                    const T x10,
                    const T x11,
                    const T alpha,
                    const T beta) {
        
        return lerp(
                    lerp(x00, x01, alpha),
                    lerp(x10, x11, alpha),
                    beta);
    }
    
    template <typename V, typename T>
    inline V bilerp(
                    const V x00,
                    const V x01,
                    const V x10,
                    const V x11,
                    const T alpha,
                    const T beta) {
        
        return lerp(
                    lerp(x00, x01, alpha),
                    lerp(x10, x11, alpha),
                    beta);
    }
    
    template <typename T>
    inline void splat(T& x0, T& x1, const T alpha, const T value) {
        x0 += value * (T(1) - alpha);
        x1 += value * alpha;
    }
    
    template <typename T>
    inline void bisplat(
                        T& x00,
                        T& x01,
                        T& x10,
                        T& x11,
                        const T alpha,
                        const T beta,
                        const T value) {
        
        const float w0 = (alpha)        * (T(1) - beta);
        const float w1 = (T(1) - alpha) * (T(1) - beta);
        const float w2 = (alpha)        * (beta);
        const float w3 = (T(1) - alpha) * (beta);
        x00 += w0 * value;
        x01 += w1 * value;
        x10 += w2 * value;
        x11 += w3 * value;
    }
    
    template <typename T>
    inline T distL2Squared(
                           const std::vector<T>& x,
                           const std::vector<T>& y) {
        
        assert(x.size() == y.size());
        T dist = 0.0;
        const int d = x.size();
        for(int i = 0; i < d; ++i) {
            const T diff = x[i] - y[i];
            dist += diff * diff;
        }
        return dist;
    }
    
    template <typename T>
    inline T distL1(
                    const std::vector<T>& x,
                    const std::vector<T>& y) {
        
        assert(x.size() == y.size());
        T dist = 0.0;
        const int d = x.size();
        for(int i = 0; i < d; ++i) {
            dist += fabs(x[i] - y[i]);
        }
        return dist;
    }
    
    const static double LOG_E = log(M_E);
    template <typename T>
    class GaussianApproximation {
    protected:
        const T xMin, xMax, yMin, yMax;
        const T xRangeRecip, yRange;
        const T sigma, scale;
        const T a0,     a2, a3; // a1 == 0
        const T b0, b1, b2, b3;
        
    public:
        GaussianApproximation(const T xMin_, const T xMax_, const T yMin_, const T yMax_) :
        xMin(xMin_),
        xMax(xMax_),
        yMin(yMin_),
        yMax(yMax_),
        xRangeRecip(2/(xMax_ - xMin_)),
        yRange(yMax_ - yMin_),
        sigma(sqrt(2.0) * 0.21),  // 0.21 instead of 0.2 to keep b(x) > 0
        scale(1 / (2*sigma*sigma)),
        a0(1),
        a2(-(-2*LOG_E*scale+12*exp(scale/4)-12)/exp(scale/4)),
        a3((-4*LOG_E*scale+16*exp(scale/4)-16)/exp(scale/4)),
        b0((2*LOG_E*scale-4)/exp(scale/4)),
        b1(-(8*LOG_E*scale-24)/exp(scale/4)),
        b2((10*LOG_E*scale-36)/exp(scale/4)),
        b3(-(4*LOG_E*scale-16)/exp(scale/4)) {}
        
        T inline operator()(const T x) const {
            const T xr = (x - xMin) * xRangeRecip - T(1);
            const T xp = xr > T(0) ? xr : -xr;
            const T yp = 
            (xp < T(1.0)) ? ((xp < T(0.5))
                             ? a0 + xp*(     xp*(a2 + xp*a3))
                             : b0 + xp*(b1 + xp*(b2 + xp*b3)))
            : T(0.0);
            return yp * yRange + yMin;
        }
    };
    
    template <typename T, typename V>
    class BezierCurve {
        protected :
        std::vector<V> points_;
        
    public:
        BezierCurve() {}
        
        BezierCurve(std::vector<V> points) {
            for (auto p : points) {
                points_.push_back(p);
            }
        }
        
        void addPoint(const V p) {
            points_.push_back(p);
        }
        
        void clearPoints() {
            points_.clear();
        }
        
        inline V operator()(const int i, const int j, const T t) const {
            return (i == j) ? points_[i]
            : lerp((*this)(i, j-1, t), (*this)(i+1, j, t), t);
        }
        
        inline V operator()(const T t) const {
            return (*this)(0, points_.size() - 1, t);
        }
    };
}

#endif /* util_hpp */
