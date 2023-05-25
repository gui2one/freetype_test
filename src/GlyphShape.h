#ifndef GLYPH_SHAPE_H
#define GLYPH_SHAPE_H
#pragma once

#include "glm/glm.hpp"


class QuadraticSegment
{

    public : 
        QuadraticSegment(glm::vec2 _pt1, glm::vec2 _ctrl, glm::vec2 _pt2 )
        {
            pt1 = _pt1;
            pt2 = _pt2;
            ctrl = _ctrl;
        }

        glm::vec2 Evaluate(float t)
        {
            if(t < 0.0f) t = 0.0f;
            else if( t > 1.0f) t = 1.0f;
            
            glm::vec2 pt = (1.0f - t) * (1.0f - t) * pt1 + 2.0f * (1.0f - t) * t * ctrl + t * t * pt2;            
            
            return pt;
        }

    private:
        glm::vec2 pt1;
        glm::vec2 pt2;
        glm::vec2 ctrl;
};

class CubicSegment
{

    public : 
        CubicSegment(glm::vec2 _pt1, glm::vec2 _ctrl1, glm::vec2 _ctrl2, glm::vec2 _pt2 )
        {
            pt1 = _pt1;
            pt2 = _pt2;
            ctrl1 = _ctrl1;
            ctrl2 = _ctrl2;
        }

        glm::vec2 Evaluate(float t)
        {       
            glm::vec2 pt =
                powf(1.f - t, 3.f) * pt1 +
                3 * powf(1.f - t, 2.f) * t * ctrl1 +
                3 * (1 - t) * powf(t, 2.f) * ctrl2 +
                pow(t, 3.f) * pt2;        
            
            return pt;
        }

    private:
        glm::vec2 pt1;

        glm::vec2 ctrl1;
        glm::vec2 ctrl2;

        glm::vec2 pt2;
};

struct MyEdge{
    glm::vec2 p1;
    glm::vec2 p2;
};
struct Contour{
    std::vector<glm::vec2> points;
};
struct GlyphShape{
    std::vector<Contour> contours;
};

#endif