/*
"C:/Windows/Fonts/ariblk.ttf"
*/
#include <iostream>
#include <ostream>
#include <sstream>
#include <msdfgen.h>
#include <msdfgen-ext.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <ft2build.h>
#include <freetype/ftoutln.h>
#include FT_FREETYPE_H


using msdfgen::Point2;
using msdfgen::EdgeHolder;
using msdfgen::EdgeSegment;
using msdfgen::QuadraticSegment;
using msdfgen::CubicSegment;
using msdfgen::Contour;

struct MyEdge{
    Point2 p1;
    Point2 p2;
};
struct MyContour{
    std::vector<Point2> points;
};
struct GlyphShape{
    std::vector<MyContour> contours;
};

std::ostream& operator<<(std::ostream& os, GlyphShape& my_shape)
{
    os << "My Shape Description ...." << std::endl;
    for(auto& contour : my_shape.contours)
    {
        os << "beginShape();" << std::endl;
        
        for(auto& point : contour.points)
        {
            os << "\tvertex(" << point.x << ", " << point.y << ");" << std::endl;
        }

        os << "endShape();\n" << std::endl;
    }
    return os;
}

std::ostream& operator<<(std::ostream& os, FT_Glyph_Metrics& metrics)
{
    os << "---- GLYPH METRICS ---------"<< std::endl;
    os << "width       : " << metrics.width << std::endl;
    os << "height      : " << metrics.height << std::endl;
    os << "horiAdvance : " << metrics.horiAdvance << std::endl;
    os << "vertAdvance : " << metrics.vertAdvance << std::endl;
    os << "horiBearing : " << metrics.horiBearingX << ", " << metrics.horiBearingY  << std::endl;
    os << "vertBearing : " << metrics.vertBearingX << ", " << metrics.vertBearingY  << std::endl;
    return os;
}

std::string write_p5_string(GlyphShape& shape,FT_Glyph_Metrics& metrics)
{
    std::ostringstream ss;
    ss << "rect(" << 0  <<", " << 0  << ", "<<  metrics.width << ", " << metrics.height << ");" << std::endl;
    ss << "translate(" << 0 << ", " << metrics.height << ");" << std::endl;

    ss << "drawGrid();" << std::endl;
    ss << "noStroke();" << std::endl;
    ss << "fill(255,255,255,20);" << std::endl;

    for(auto& contour : shape.contours)
    {
        ss << "beginShape();" << std::endl;
        
        for(auto& point : contour.points)
        {
            ss << "\tvertex(" << point.x << ", " << point.y << ");" << std::endl;
        }

        ss << "endShape();\n" << std::endl;
    }

    return ss.str();
} 
int outlineMoveTo(const FT_Vector* to, void* user) {
    GlyphShape* shape = reinterpret_cast<GlyphShape*>(user);
    MyContour contour;
    contour.points.push_back(Point2(to->x, to->y * -1.0f));
    shape->contours.push_back(contour);
    return 0;
}

int outlineLineTo(const FT_Vector* to, void* user) {
    GlyphShape* shape = reinterpret_cast<GlyphShape*>(user);  
    auto& contour = shape->contours.back();
    contour.points.push_back(Point2(to->x, to->y * -1.0f));
    return 0;
}

int outlineConicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
    GlyphShape* shape = reinterpret_cast<GlyphShape*>(user);
    auto& contour = shape->contours.back();
    auto pt0 = contour.points.back();
    QuadraticSegment quad_segment(pt0, Point2(control->x, control->y * -1.0f), Point2(to->x, to->y * -1.0f));

    size_t iterations = 10;
    for(size_t i=0; i<iterations; i++)
    {
        double step = 1.0 / ((double)iterations);
        auto pt = quad_segment.point(step * (i+1));
        contour.points.push_back(pt);
    }

    return 0;
}

int outlineCubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) {
    GlyphShape* shape = reinterpret_cast<GlyphShape*>(user);
    auto& contour = shape->contours.back();
    auto pt0 = contour.points.back();
    CubicSegment cubic_segment(pt0, Point2(control1->x, control1->y * -1.0f), Point2(control2->x, control2->y * -1.0f), Point2(to->x, to->y * -1.0f));

    size_t iterations = 10;
    for(size_t i=0; i<iterations; i++)
    {
        double step = 1.0 / ((double)iterations);
        auto pt = cubic_segment.point(step * (i+1));
        contour.points.push_back(pt);
    }

    return 0;
}

int main() {
    // Initialize FreeType library
    FT_Library ftLibrary;
    FT_Init_FreeType(&ftLibrary);

    // Load font face
    FT_Face ftFace;
    FT_New_Face(ftLibrary, "C:/Windows/Fonts/ariblk.ttf", 0, &ftFace);

    // Set font size and scaling
    FT_Set_Pixel_Sizes(ftFace, 3, 3); // Adjust the size as needed

    // Load glyph into the face's glyph slot
    FT_Load_Glyph(ftFace, FT_Get_Char_Index(ftFace, 'j'), FT_LOAD_DEFAULT);

    auto metrics = ftFace->glyph->metrics;
    // Convert the glyph outline to an msdfgen shape
    msdfgen::Shape shape;
    GlyphShape my_shape;
    FT_Outline_Funcs outlineFuncs = { outlineMoveTo, outlineLineTo, outlineConicTo, outlineCubicTo, 0, 0 };
    FT_Outline_Decompose(&ftFace->glyph->outline, &outlineFuncs, &my_shape);

    // std::cout << my_shape << std::endl;
    
    auto p5_data = write_p5_string(my_shape, ftFace->glyph->metrics);

    std::cout << p5_data << std::endl;
    std::cout << ftFace->glyph->metrics << std::endl;
    
    // Apply edge coloring
    msdfgen::edgeColoringSimple(shape, 3.0);

    // Set the size of the output image
    int width = 256;
    int height = 256;

    // Create an empty bitmap
    msdfgen::Bitmap<float, 3> bitmap(width, height);

    // Generate the signed distance field
    msdfgen::generateMSDF(bitmap, shape, 4.0, msdfgen::Vector2(-1.0, -1.0), msdfgen::Vector2(1.0, 1.0));

    // Convert the bitmap to RGBA format
    std::vector<unsigned char> pixels(width * height * 4);
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int index = (y * width + x) * 4;
            pixels[index] = static_cast<unsigned char>(bitmap(x, y)[0] * 255); // Red channel
            pixels[index + 1] = static_cast<unsigned char>(bitmap(x, y)[1] * 255); // Green channel
            pixels[index + 2] = static_cast<unsigned char>(bitmap(x, y)[2] * 255); // Blue channel
            pixels[index + 3] = 255;  // Alpha value
        }
    }

    // Save the bitmap to a file using stb_image
    stbi_write_png("output.png", width, height, 4, pixels.data(), width * 4);

    // Clean up FreeType resources
    FT_Done_Face(ftFace);
    FT_Done_FreeType(ftLibrary);

    std::cout << "MSDF generation complete." << std::endl;

    return 0;
}