// NNModel.hpp

#pragma once

#include "backend/backend.h"

#include <map>
#include <ostream>
#include <vector>

namespace NN {

// store info about a model method
class NNModelMethod
{
public:
    // read method params from model method's params
    NNModelMethod(std::string const& name, std::vector<int> const& params);

    std::string name;
    int inDim, inRatio, outDim, outRatio;
};

enum NNAttributeType
{
    typeBool,
    typeInt,
    typeDouble,
    typeOther
};
struct NNModelAttribute
{
    NNAttributeType type;
    std::string name;
};

// read and store model information
// needed mostly to avoid passing strings to UGens
class NNModelDesc
{
public:
    NNModelDesc(unsigned short id);

    // load .ts, just to read info
    bool load(char const* path);

    NNModelMethod const* getMethod(unsigned short idx, bool warn = true) const;
    NNModelAttribute const* getAttribute(unsigned short idx, bool warn = true) const;

    // info
    bool is_loaded() const { return m_loaded; }
    void streamInfo(std::ostream& dest) const;
    bool dumpInfo(char const* filename) const;
    void printInfo() const;
    int getHigherRatio() const { return m_higherRatio; }
    char const* getPath() const { return m_path.c_str(); }
    Backend const& backend() const { return m_backend; }

private:
    Backend m_backend;
    std::vector<NNModelMethod> m_methods;
    std::vector<NNModelAttribute> m_attributes;
    int m_higherRatio;
    unsigned short m_idx;
    bool m_loaded = false;
    std::string m_path;
};

// register model info by int id
// used as a global NNModelDesc store
class NNModelDescLib
{
public:
    NNModelDescLib();
    // load model from .ts file
    NNModelDesc* load(char const* path);
    NNModelDesc* load(unsigned short id, char const* path);
    void unload(unsigned short id);
    /* void reload(unsigned short id); */

    // get stored model
    NNModelDesc* get(unsigned short id, bool warn = true) const;
    unsigned short findId(char const* path);
    // all loaded models info
    void streamAllInfo(std::ostream& stream) const;
    bool dumpAllInfo(char const* filename) const;
    void printAllInfo() const;

private:
    unsigned short getNextId();
    std::map<unsigned short, NNModelDesc*> models;
    unsigned short modelCount;
};

} // namespace NN
