#ifndef _FIELD_IMAGE_SET_H_
#define _FIELD_IMAGE_SET_H_ 1

#include <vector>
#include <memory>
#include <utility>
#include "glm/glm.hpp"
#include "Variant.h"
#include "FieldImage.h"


class FieldImageSet{
    public:
        std::vector<FieldImage> set ;

    FieldImageSet();

    void addImage(FieldImage& image);

    glm::vec3 getColor(const glm::vec3 &p, const glm::vec3 &v);
    
};
#endif // #ifndef _FIELD_IMAGE_SET_H_