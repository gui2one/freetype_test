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

// #include <iomanip>
#include <locale>
#include <string>
#include <codecvt>



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

        for(const auto& base_shape : base_shapes)
        {

            for (size_t i = 0; i < base_shape.size(); i++)
            {
                delete base_shape[i];
            }

        }
        for(auto& hole : holes)
        {
            for (size_t i = 0; i < hole.size(); i++)
            {
                delete hole[i];
            }
        }

        std::cout << "-- Poly2TriShape::DESTRUCTOR Called" << std::endl;
    }


    std::vector<std::vector<p2t::Point*>> base_shapes;
    std::vector<std::vector<p2t::Point*>> holes;

};

Poly2TriShape* glyph_shape_to_poly2tri(const GlyphShape& glyph_shape)
{
    Poly2TriShape* s = new Poly2TriShape();

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

        s->base_shapes.push_back(points);
         
    }

    if( glyph_shape.contours.size() > 1)
    {
        for (size_t i = 1; i < glyph_shape.contours.size(); i++)
        {
            /* code */
            auto& contour = glyph_shape.contours[i];

            std::vector<p2t::Point*> points;
            for (auto& cpt : contour.points)
            {
                p2t::Point* pt = new p2t::Point(cpt.x, cpt.y);
                points.push_back(pt);
            }

            // remove last point !!
            points.pop_back();

            if(!contour_is_clockwise(contour)){
                s->holes.push_back(points);
            }else{
                s->base_shapes.push_back(points);
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
    FT_New_Face(ftLibrary, "C:/Windows/Fonts/arial.ttf", 0, &ftFace);


    // Load glyph into the face's glyph slot
    FT_Select_Charmap(ftFace, ft_encoding_unicode);

    // Set font size and scaling
    FT_Set_Pixel_Sizes(ftFace, 3, 3); // Adjust the size as needed


    auto char_index = FT_Get_Char_Index(ftFace, 'i');
    FT_Load_Glyph(ftFace, char_index, FT_LOAD_DEFAULT);
    auto metrics = ftFace->glyph->metrics;
    // Convert the glyph outline to an msdfgen shape

    GlyphShape my_shape;
    FT_Outline_Funcs outlineFuncs = { outlineMoveTo, outlineLineTo, outlineConicTo, outlineCubicTo, 0, 0 };
    FT_Outline_Decompose(&ftFace->glyph->outline, &outlineFuncs, &my_shape);




    Poly2TriShape* poly_shape = glyph_shape_to_poly2tri(my_shape);

    std::vector<glm::vec2> points; 
    std::vector<int> indices;
    std::vector<p2t::Triangle *> triangles;

    for(const auto& base_shape : poly_shape->base_shapes)
    {

        p2t::CDT* cdt = new p2t::CDT(base_shape);
        for(const auto& hole : poly_shape->holes)
        {
            cdt->AddHole(hole);
        }

        cdt->Triangulate();
        auto tris = cdt->GetTriangles();
        triangles.insert(triangles.end(), tris.begin(), tris.end());



    }



    auto mesh_data = convertTrianglesToPointsAndIndices(triangles);

    std::cout << "num points  : " <<mesh_data.first.size() << std::endl;
    std::cout << "num indices : " <<mesh_data.second.size() << std::endl;

    points = mesh_data.first;
    indices = mesh_data.second;


    delete poly_shape;
    
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


    /* 
        very cool method, used only to populate clipboard to be able to just paste content into p5js editor, 
        for visualization purposes. thanks to chatgpt :)
    */
    SetClipboardText(ss.str());

    // Clean up FreeType resources
    FT_Done_Face(ftFace);
    FT_Done_FreeType(ftLibrary);

    std::cout << "Mesh generation complete." << std::endl;

    return 0;
}