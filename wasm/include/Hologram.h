#ifndef _HOLOGRAM_H_
#define _HOLOGRAM_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"
#include "Variant.h"
#include "HologramPanel.h"
#include "HologramView.h"

class Hologram{
    public:
        glm::vec3 vantage ; // expected vantage area
        float vantage_radius ;

        std::vector<HologramPanel> panel;
        std::vector<HologramView> view;
        
        // position of upper left corner of image, how pixel coordinates convert to 3D space and normal of image plane (typically facing toward camera)
        Hologram(glm::vec3 view_point, float view_radius);

        // Adds a new panel for determining depth of view rays
        void addPanel(const HologramPanel& p);

        // Constructs and adds a new panel
        void addPanel(glm::vec3 zero, glm::vec3 xperpixel, glm::vec3 yperpixel , glm::vec3 z_normal, std::vector<std::vector<float>>& depth, int block_size);

        // Adds a new image view which will be reprojected to produce output colors
        void addView(const HologramView& v);

        // Constructs and adds a new view
        // Note: image_data will be moved and passed in Variant will be destroyed
        void addView(glm::vec3 p, glm::vec3 n , const std::vector<std::pair<glm::vec3, glm::vec2>> &v_to_pixel, Variant& image_data, int w, int h);

        // get the point a ray would hit by intersecting it with the appropriate panel
        glm::vec3 getPoint(const glm::vec3 &p, const glm::vec3 &v);

        // get the color of a ray
        glm::vec3 getColor(const glm::vec3 &p, const glm::vec3 &v);

};
#endif // #ifndef _HOLOGRAM_H_