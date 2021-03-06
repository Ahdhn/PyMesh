#include "SVGParser.h"

#define NANOSVG_IMPLEMENTATION
#include "nanosvg.h"

using namespace PyMesh;

void SVGParser::parse(const std::string& filename) {
    NSVGimage* image = nsvgParseFromFile(filename.c_str(), "px", 96);
    const auto width = image->width;
    const auto height = image->height;
    const auto tol = Float(width + height) * 1.e-4;

    for (auto shape = image->shapes; shape != NULL; shape=shape->next) {
        for (auto path = shape->paths; path != NULL; path=path->next) {
            const int v_first = m_vertices.size();
            for (size_t i=0; i<path->npts-1; i+=3) {
                float* p = &path->pts[i*2];
                const Vector2F p0{p[0], p[1]};
                const Vector2F p1{p[2], p[3]};
                const Vector2F p2{p[4], p[5]};
                const Vector2F p3{p[6], p[7]};
                add_bezier_curve(p0, p1, p2, p3, tol, 0, i!=0);
            }
            const int v_last = m_vertices.size()-1;
            if (path->closed) {
                m_edges.emplace_back(v_last, v_first);
            }
        }
    }
    nsvgDelete(image);

    remove_duplicate_points();
}


void SVGParser::add_bezier_curve(
        const Vector2F& p0,
        const Vector2F& p1,
        const Vector2F& p2,
        const Vector2F& p3, Float tol, int level, bool start_with_previous) {
    const Vector2F p01   = 0.5 * (p0 + p1);
    const Vector2F p12   = 0.5 * (p1 + p2);
    const Vector2F p23   = 0.5 * (p2 + p3);
    const Vector2F p012  = 0.5 * (p01 + p12);
    const Vector2F p123  = 0.5 * (p12 + p23);
    const Vector2F p0123 = 0.5 * (p012 + p123);

    const Float diff = (p0123 - p12).norm();
    if (diff < tol || level >= 6) {
        const int vid = m_vertices.size();

        if (start_with_previous) {
            assert(vid > 0);
            m_vertices.push_back(p0123);
            m_vertices.push_back(p3);
            m_edges.emplace_back(vid-1, vid);
            m_edges.emplace_back(vid, vid+1);
        } else {
            m_vertices.push_back(p0);
            m_vertices.push_back(p0123);
            m_vertices.push_back(p3);

            m_edges.emplace_back(vid, vid+1);
            m_edges.emplace_back(vid+1, vid+2);
        }
    } else {
        add_bezier_curve(p0, p01, p012, p0123, tol, level+1, start_with_previous);
        add_bezier_curve(p0123, p123, p23, p3, tol, level+1, true);
    }
}

void SVGParser::remove_duplicate_points() {
}

void SVGParser::export_vertices(Float* buffer) const {
    size_t i=0;
    for (const auto& v : m_vertices) {
        buffer[i*2] = v[0];
        buffer[i*2+1] = v[1];
        i++;
    }
}

void SVGParser::export_edges(int* buffer) const {
    size_t i=0;
    for (const auto& e : m_edges) {
        buffer[i*2] = e[0];
        buffer[i*2+1] = e[1];
        i++;
    }
}
