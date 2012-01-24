#ifndef MOVIE_CONTENT_H
#define MOVIE_CONTENT_H

#include "Content.h"
#include <boost/serialization/base_object.hpp>
#include <boost/serialization/export.hpp>

class MovieContent : public Content {

    public:
        MovieContent(std::string uri = "") : Content(uri) { }

        void getFactoryObjectDimensions(int &width, int &height);

    private:
        friend class boost::serialization::access;

        template<class Archive>
        void serialize(Archive & ar, const unsigned int)
        {
            // serialize base class information
            ar & boost::serialization::base_object<Content>(*this);
        }

        void advance(boost::shared_ptr<ContentWindowManager> window);

        void renderFactoryObject(float tX, float tY, float tW, float tH);
};

#endif
