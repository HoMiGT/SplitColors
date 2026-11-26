#include "split_colors.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <stdexcept>
#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace split {
bool isImage(const fs::path& path) {
    auto ext=path.extension().string();
    std::transform(ext.begin(),ext.end(),ext.begin(),[](unsigned char c){return static_cast<char>(std::tolower(c));});
    return ext==".jpg" || ext==".jpeg" || ext==".png" || ext==".bmp" || ext==".webp" || ext==".tif" || ext==".tiff";
}
std::vector<fs::path> findImages(const fs::path& directory) {
    std::vector<fs::path> result;
    for (const auto& item:fs::directory_iterator(directory))
        if (item.is_regular_file() && isImage(item.path())) result.push_back(item.path());
    std::sort(result.begin(),result.end());
    return result;
}
cv::Mat readImage(const fs::path& path) {
    std::ifstream in(path,std::ios::binary);
    if (!in) throw std::runtime_error("Cannot open image: "+utf8(path));
    std::vector<unsigned char> bytes((std::istreambuf_iterator<char>(in)),{});
    if (bytes.empty()) throw std::runtime_error("Empty image: "+utf8(path));
    cv::Mat image=cv::imdecode(bytes,cv::IMREAD_UNCHANGED);
    if (image.empty() || image.depth()!=CV_8U) throw std::runtime_error("Invalid image or unsupported depth (only 8-bit supported): "+utf8(path));
    cv::Mat result;
    if (image.channels()==4) return image;
    if (image.channels()==3) cv::cvtColor(image,result,cv::COLOR_BGR2BGRA);
    else if (image.channels()==1) cv::cvtColor(image,result,cv::COLOR_GRAY2BGRA);
    else throw std::runtime_error("Unsupported image channels: "+utf8(path));
    return result;
}
void writePng(const fs::path& path,const cv::Mat& image) {
    std::vector<unsigned char> bytes;
    if (!cv::imencode(".png",image,bytes)) throw std::runtime_error("PNG encoding failed: "+utf8(path));
    std::ofstream out(path,std::ios::binary);
    if (!out || !out.write(reinterpret_cast<const char*>(bytes.data()),static_cast<std::streamsize>(bytes.size())))
        throw std::runtime_error("Cannot write image: "+utf8(path));
}
static bool looksColorful(const cv::Mat& image) {
    std::uint64_t visible=0,colored=0;
    int step=std::max(1,static_cast<int>(std::sqrt(static_cast<double>(image.total())/100000.0)));
    for (int y=0;y<image.rows;y+=step) for (int x=0;x<image.cols;x+=step) {
        auto p=image.at<cv::Vec4b>(y,x);
        if (!p[3]) continue;
        ++visible;
        if (std::max({p[0],p[1],p[2]})-std::min({p[0],p[1],p[2]})>20) ++colored;
    }
    return visible && colored*100>=visible;
}
fs::path chooseSource(const std::vector<fs::path>& images,const Config& config) {
    if (images.size()<2) throw std::runtime_error("Each group needs one source image and at least one mask");
    if (!config.sourceName.empty()) {
        for (const auto& p:images) if (p.filename()==config.sourceName) return p;
        throw std::runtime_error("SC_SOURCE_IMAGE was not found in this group: "+utf8(config.sourceName));
    }
    fs::path source;
    for (const auto& p:images) if (looksColorful(readImage(p))) {
        if (!source.empty()) throw std::runtime_error("More than one colorful image found; specify SC_SOURCE_IMAGE");
        source=p;
    }
    if (source.empty()) throw std::runtime_error("Cannot identify source image; specify SC_SOURCE_IMAGE");
    return source;
}
static cv::Mat maskOpacity(const fs::path& path,const cv::Size& size,const Config& config) {
    cv::Mat image=readImage(path);
    double aspect=static_cast<double>(image.cols)*size.height/(static_cast<double>(image.rows)*size.width);
    if (std::abs(aspect-1.0)>0.01) throw std::runtime_error("Mask aspect ratio differs from source: "+utf8(path));
    std::vector<cv::Mat> channels;
    cv::split(image,channels);
    double minAlpha=0;
    cv::minMaxLoc(channels[3],&minAlpha,nullptr);
    cv::Mat opacity;
    if (minAlpha<255) opacity=channels[3];
    else {
        cv::cvtColor(image,opacity,cv::COLOR_BGRA2GRAY);
        if (config.blackForeground) cv::bitwise_not(opacity,opacity);
        // Remove small JPEG deviations near white/black, keeping the edge ramp.
        if (config.softEdges) opacity.convertTo(opacity,CV_8U,255.0/239.0,-8.0*255.0/239.0);
    }
    if (opacity.size()!=size) {
        cv::Mat resized;
        cv::resize(opacity,resized,size,0,0,(opacity.cols>size.width || opacity.rows>size.height) ? cv::INTER_AREA : cv::INTER_LINEAR);
        opacity=resized;
    }
    if (!config.softEdges) cv::threshold(opacity,opacity,config.threshold-1,255,cv::THRESH_BINARY);
    return opacity;
}
int splitGroup(const fs::path& sourcePath,const std::vector<fs::path>& masks,const fs::path& output,const Config& config) {
    cv::Mat source=readImage(sourcePath);
    std::uint64_t visible=0;
    for (int y=0;y<source.rows;++y) for (int x=0;x<source.cols;++x) if (source.at<cv::Vec4b>(y,x)[3]) ++visible;
    if (!visible) throw std::runtime_error("Source image is fully transparent");
    fs::create_directories(output/"layers");
    for (size_t i=0;i<masks.size();++i) {
        cv::Mat opacity=maskOpacity(masks[i],source.size(),config);
        cv::Mat layer(source.size(),CV_8UC4,cv::Scalar(0,0,0,0));
        for (int y=0;y<source.rows;++y) for (int x=0;x<source.cols;++x) {
            auto pixel=source.at<cv::Vec4b>(y,x);
            unsigned alpha=(static_cast<unsigned>(pixel[3])*opacity.at<uchar>(y,x)+127)/255;
            if (!alpha) continue;
            pixel[3]=static_cast<uchar>(alpha); layer.at<cv::Vec4b>(y,x)=pixel;
        }
        std::ostringstream number; number << std::setfill('0') << std::setw(3) << i+1;
        fs::path name=fs::path(number.str()+"_"); name+=masks[i].stem().native(); name+=".png";
        writePng(output/"layers"/name,layer);
    }
    return static_cast<int>(masks.size());
}
}
