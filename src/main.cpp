/*
"C:/Windows/Fonts/ariblk.ttf"
*/
#include <iostream>
#include <ostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <windows.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include <ft2build.h>
#include <freetype/ftoutln.h>
#include FT_FREETYPE_H

#include "glm/glm.hpp"
#include "GlyphShape.h"

#include "poly2tri/poly2tri.h"


size_t ITER = 3;
float contour_is_clockwise(const Contour& contour);


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
        
        if(contour_is_clockwise(contour))
        {
            ss << "fill(255,0,0,20);" << std::endl;
        }else{
            ss << "fill(0,255,0,20);" << std::endl;
        }

        ss << "beginShape();" << std::endl;
        
        for(auto& point : contour.points)
        {
            ss << "\tvertex(" << point.x << ", " << point.y << ");" << std::endl;
        }

        ss << "endShape();\n" << std::endl;
    }

    return ss.str();
} 

float contour_is_clockwise(const Contour& contour) {
    float sum = 0.0f;
    size_t numPoints = contour.points.size();

    for (size_t i = 0; i < numPoints; ++i) {
        const glm::vec2& p1 = contour.points[i];
        const glm::vec2& p2 = contour.points[(i + 1) % numPoints];
        sum += (p2.x - p1.x) * (p2.y + p1.y);
    }

    // Positive sum indicates counterclockwise winding
    // Negative sum indicates clockwise winding
    return sum < 0;
}

// Function to set text in the clipboard
void SetClipboardText(const std::string& text)
{
    // Open the clipboard
    if (!OpenClipboard(nullptr))
    {
        // Handle error
        return;
    }

    // Empty the clipboard
    EmptyClipboard();

    // Get the length of the text
    size_t textLength = text.length();

    // Allocate global memory for the text
    HGLOBAL hText = GlobalAlloc(GMEM_MOVEABLE, textLength + 1);
    if (hText == nullptr)
    {
        // Handle error
        CloseClipboard();
        return;
    }

    // Lock the global memory
    char* pText = static_cast<char*>(GlobalLock(hText));
    if (pText == nullptr)
    {
        // Handle error
        GlobalFree(hText);
        CloseClipboard();
        return;
    }

    // Copy the text to the global memory
    memcpy(pText, text.c_str(), textLength);
    pText[textLength] = '\0'; // Add null-terminator

    // Unlock the global memory
    GlobalUnlock(hText);

    // Set the text in the clipboard
    SetClipboardData(CF_TEXT, hText);

    // Close the clipboard
    CloseClipboard();
}


int outlineMoveTo(const FT_Vector* to, void* user) {
    GlyphShape* shape = reinterpret_cast<GlyphShape*>(user);
    Contour contour;
    contour.points.push_back(glm::vec2(to->x, to->y * -1.0f));
    shape->contours.push_back(contour);
    return 0;
}

int outlineLineTo(const FT_Vector* to, void* user) {
    GlyphShape* shape = reinterpret_cast<GlyphShape*>(user);  
    auto& contour = shape->contours.back();
    contour.points.push_back(glm::vec2(to->x, to->y * -1.0f));
    return 0;
}

int outlineConicTo(const FT_Vector* control, const FT_Vector* to, void* user) {
    GlyphShape* shape = reinterpret_cast<GlyphShape*>(user);
    auto& contour = shape->contours.back();
    auto pt0 = contour.points.back();
    QuadraticSegment quad_segment(pt0, glm::vec2(control->x, control->y * -1.0f), glm::vec2(to->x, to->y * -1.0f));

    size_t iterations = ITER;
    for(size_t i=0; i<iterations; i++)
    {
        double step = 1.0 / ((double)iterations);
        auto pt = quad_segment.Evaluate(step * (i+1));
        contour.points.push_back(pt);
    }

    return 0;
}

int outlineCubicTo(const FT_Vector* control1, const FT_Vector* control2, const FT_Vector* to, void* user) {
    GlyphShape* shape = reinterpret_cast<GlyphShape*>(user);
    auto& contour = shape->contours.back();
    auto pt0 = contour.points.back();
    CubicSegment cubic_segment(pt0, glm::vec2(control1->x, control1->y * -1.0f), glm::vec2(control2->x, control2->y * -1.0f), glm::vec2(to->x, to->y * -1.0f));

    size_t iterations = ITER;
    for(size_t i=0; i<iterations; i++)
    {
        double step = 1.0 / ((double)iterations);
        auto pt = cubic_segment.Evaluate(step * (i+1));
        contour.points.push_back(pt);
    }

    return 0;
}


/* Utiliies */
class Poly2TriShape
{

    public :
    Poly2TriShape(){

    }
    ~Poly2TriShape(){
        // for (size_t i = 0; i < base_shape.size(); i++)
        // {
        //     delete base_shape[i];
        // }

        // for(auto& hole : holes)
        // {
        //     for (size_t i = 0; i < hole.size(); i++)
        //     {
        //         delete hole[i];
        //     }
        // }

        std::cout << "-- Poly2TriShape::DESTRUCTOR Called" << std::endl;
    }


    std::vector<p2t::Point*> base_shape;
    std::vector<std::vector<p2t::Point*>> holes;

};

Poly2TriShape glyph_shape_to_poly2tri(const GlyphShape& glyph_shape)
{
    Poly2TriShape s;

    if(glyph_shape.contours.size() > 0)
    {
        auto& contour = glyph_shape.contours[0];
        std::vector<p2t::Point*> points;
        // points.reserve(contour.points.size());

        for (auto& cpt : contour.points)
        {
            p2t::Point* pt = new p2t::Point(cpt.x, cpt.y);
            points.push_back(pt);

            // std::cout << pt->x << ", " << pt->y << std::endl;
            
        }

        // remove last point !!
        points.pop_back();

        s.base_shape = points;
         
    }

    if( glyph_shape.contours.size() > 1)
    {
        for (size_t i = 1; i < glyph_shape.contours.size(); i++)
        {
            /* code */
            auto& contour = glyph_shape.contours[i];

            if(!contour_is_clockwise(contour)){
                std::vector<p2t::Point*> points;
                for (auto& cpt : contour.points)
                {
                    p2t::Point* pt = new p2t::Point(cpt.x, cpt.y);
                    points.push_back(pt);

                    // std::cout << pt->x << ", " << pt->y << std::endl;
                    
                }

                // remove last point !!
                points.pop_back();
                s.holes.push_back(points);
            }
        }
        
    }
    return s;
}

std::pair<std::vector<glm::vec2>, std::vector<int>> convertTrianglesToPointsAndIndices(const std::vector<p2t::Triangle *>& triangles) {
    std::vector<glm::vec2> uniquePoints;
    std::vector<int> indices;
    std::unordered_map<p2t::Point*, int> pointToIndexMap;

    
    for (const auto& triangle : triangles) {
        for (int i = 0; i < 3; ++i) {
            
            auto vertex = triangle->GetPoint(i);
            auto it = pointToIndexMap.find(vertex);
            if (it != pointToIndexMap.end()) {
                // Vertex already exists, retrieve the index
                indices.push_back(it->second);
            } else {
                // Add the vertex to the map and unique points vector
                int newIndex = static_cast<int>(uniquePoints.size());
                pointToIndexMap[vertex] = newIndex;
                uniquePoints.push_back(glm::vec2(vertex->x, vertex->y));
                indices.push_back(newIndex);
            }
        }
    }

    return std::make_pair(uniquePoints, indices);
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
    FT_Load_Glyph(ftFace, FT_Get_Char_Index(ftFace, '$'), FT_LOAD_DEFAULT);

    auto metrics = ftFace->glyph->metrics;
    // Convert the glyph outline to an msdfgen shape

    GlyphShape my_shape;
    FT_Outline_Funcs outlineFuncs = { outlineMoveTo, outlineLineTo, outlineConicTo, outlineCubicTo, 0, 0 };
    FT_Outline_Decompose(&ftFace->glyph->outline, &outlineFuncs, &my_shape);




    Poly2TriShape poly_shape = glyph_shape_to_poly2tri(my_shape);
    // poly_shape.base_shape = { 
    //     new p2t::Point(0.0, 0.0), 
    //     new p2t::Point(1.0, 0.0), 
    //     new p2t::Point(1.0, 1.0), 
    //     new p2t::Point(0.0, 1.0) 
    // };
    p2t::CDT* cdt = new p2t::CDT(poly_shape.base_shape);
    for(const auto& hole : poly_shape.holes)
    {
        cdt->AddHole(hole);
    }

    cdt->Triangulate();
    auto triangles = cdt->GetTriangles();

    


    auto mesh_data = convertTrianglesToPointsAndIndices(triangles);

    std::cout << "num points  : " <<mesh_data.first.size() << std::endl;
    std::cout << "num indices : " <<mesh_data.second.size() << std::endl;

    auto& points = mesh_data.first;
    auto& indices = mesh_data.second;








    std::stringstream ss;

    // write indices
    ss << "let indices = [";
    
    size_t num_indices = indices.size();
    for (size_t i = 0; i < num_indices; i++)
    {
        ss << indices[i];
        if( i < num_indices-1)
        {
            ss << ", ";
        }
    }
    ss << "];" << std::endl;

    // write points
    ss << "\nlet points = [";
    
    size_t num_points = points.size();
    for (size_t i = 0; i < num_points; i++)
    {
        ss << points[i].x << ", " << points[i].y;
        if( i < num_points-1)
        {
            ss << ", ";
        }
    }
    ss << "];" << std::endl;


    // for(const auto& tri : triangles)
    // {
    //     ss << "beginShape();";

    //     ss << "vertex(" <<tri->GetPoint(0)->x << ","  << tri->GetPoint(0)->y << ");";
    //     ss << "vertex(" <<tri->GetPoint(1)->x << ","  << tri->GetPoint(1)->y << ");";
    //     ss << "vertex(" <<tri->GetPoint(2)->x << ","  << tri->GetPoint(2)->y << ");";
        
    //     ss << "endShape();";
    // }
    SetClipboardText(ss.str());
    // std::cout << my_shape << std::endl;
    
    // auto p5_data = write_p5_string(my_shape, ftFace->glyph->metrics);

    // std::cout << p5_data << std::endl;
    // std::cout << ftFace->glyph->metrics << std::endl;
    

    // msdfgen::Shape shape = glyph_shape_to_msdfgen_shape(my_shape);
    // // Apply edge coloring
    // msdfgen::edgeColoringSimple(shape, 3.0);

    // // Set the size of the output image
    // int width = 256;
    // int height = 256;

    // // Create an empty bitmap
    // msdfgen::Bitmap<float, 3> bitmap(width, height);

    // // Generate the signed distance field
    // msdfgen::generateMSDF(bitmap, shape, 32.0, msdfgen::Vector2(1.0, 1.0), msdfgen::Vector2(0.0, 192.0));

    // // Convert the bitmap to RGBA format
    // std::vector<unsigned char> pixels(width * height * 3);
    // for (int y = 0; y < height; ++y) {
    //     for (int x = 0; x < width; ++x) {
    //         int index = (y * width + x) * 3;
    //         pixels[index] = static_cast<unsigned char>(bitmap(x, y)[0] * 255); // Red channel
    //         pixels[index + 1] = static_cast<unsigned char>(bitmap(x, y)[1] * 255); // Green channel
    //         pixels[index + 2] = static_cast<unsigned char>(bitmap(x, y)[2] * 255); // Blue channel
    //         // pixels[index + 3] = 255;  // Alpha value
    //     }
    // }

    // // Save the bitmap to a file using stb_image
    // stbi_write_png("output2.png", width, height, 3, pixels.data(), width * 3);

    // Clean up FreeType resources
    FT_Done_Face(ftFace);
    FT_Done_FreeType(ftLibrary);

    std::cout << "MSDF generation complete." << std::endl;

    return 0;
}