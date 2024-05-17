#include "NNModel.hpp"

#include "SC_InterfaceTable.h"
#undef Print

#include "backend/backend.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <ostream>

extern InterfaceTable* ft;
#define Print (*ft->fPrint)

namespace NN {

NNModelDesc::NNModelDesc(unsigned short id)
: m_idx(id)
{
}

bool NNModelDesc::load(char const* path)
{
    Print("NNModelDesc: loading %s\n", path);
    Backend backend;
    bool loaded = backend.load(path) == 0;
    if (loaded)
    {
        Print("NNModelDesc: loaded %s\n", path);
    }
    else
    {
        Print("ERROR: NNModelDesc can't load model %s\n", path);
        return false;
    }

    // cache path
    m_path = path;

    m_higherRatio = backend.get_higher_ratio();

    // cache methods
    if (m_methods.size() > 0)
        m_methods.clear();
    for (std::string const& name : backend.get_available_methods())
    {
        auto params = backend.get_method_params(name);
        // skip methods with no params
        if (params.size() == 0)
            continue;
        m_methods.push_back({name, params});
    }

    // cache attributes
    if (m_attributes.size() > 0)
        m_attributes.clear();
    for (std::string const& name : backend.get_settable_attributes())
    {
        try
        {
            c10::IValue value = backend.get_attribute(name)[0];
            NNAttributeType attrType;
            if (value.isBool())
                attrType = NNAttributeType::typeBool;
            else if (value.isInt())
                attrType = NNAttributeType::typeInt;
            else if (value.isDouble())
                attrType = NNAttributeType::typeDouble;
            else
                attrType = NNAttributeType::typeOther;
            /* Print("attr %s %d\n", name.c_str(), attrType); */
            m_attributes.push_back({attrType, name});
        }
        catch (...)
        {
            Print("NNModelDesc: couldn't read attribute '%s'\n", name.c_str());
        }
    }

    m_loaded = true;
    return true;
}

NNModelMethod const* NNModelDesc::getMethod(unsigned short idx, bool warn) const
{
    try
    {
        return &m_methods.at(idx);
    }
    catch (std::out_of_range const&)
    {
        if (warn)
            Print("NNModelDesc: method %d not found\n", idx);
        return nullptr;
    }
}

NNModelAttribute const* NNModelDesc::getAttribute(unsigned short idx, bool warn) const
{
    try
    {
        return &m_attributes.at(idx);
    }
    catch (std::out_of_range const&)
    {
        if (warn)
            Print("NNBackend: attribute %d not found\n", idx);
        return nullptr;
    }
}

NNModelMethod::NNModelMethod(std::string const& name, std::vector<int> const& params)
: name(name)
{
    inDim = params[0];
    inRatio = params[1];
    outDim = params[2];
    outRatio = params[3];
}

NNModelDescLib::NNModelDescLib()
: models()
, modelCount(0)
{
}

unsigned short NNModelDescLib::getNextId()
{
    unsigned short id = modelCount;
    while (models[id] != nullptr)
        id++;
    return id;
};

NNModelDesc* NNModelDescLib::get(unsigned short id, bool warn) const
{
    NNModelDesc* model;
    bool found = false;
    try
    {
        model = models.at(id);
        found = model != nullptr;
    }
    catch (...)
    {
        if (warn)
        {
            Print(
                "NNModelDescLib: id %d not found. Loaded models:%s\n",
                id,
                models.size() ? "" : " []"
            );
            for (auto kv : models)
            {
                Print("id: %d -> %s\n", kv.first, kv.second->getPath());
            }
        };
        found = false;
    }

    if (!found)
        return nullptr;
    if (!model->is_loaded())
    {
        if (warn)
            Print("NNModelDescLib: id %d not loaded yet\n", id);
    }
    return model;
}

void NNModelDescLib::streamAllInfo(std::ostream& dest) const
{
    for (auto const& kv : models)
    {
        kv.second->streamInfo(dest);
    }
}

void NNModelDescLib::printAllInfo() const
{
    streamAllInfo(std::cout);
    std::cout << std::endl;
}

unsigned short NNModelDescLib::findId(char const* path)
{
    auto it = std::find_if(std::begin(models), std::end(models), [&path](auto& p) {
        return strcmp(p.second->getPath(), path) == 0;
    });
    // if not found return 65535 (instead of -1 because unsigned short)
    return (it == std::end(models)) ? 65535 : it->first;
}

// load:
// check if model info are already loaded when loading without providing id
// when id is provided, don't check and load again to another id
// because the user must know what they're doing:
// if user loads a model to a specific id, that model should
// be available at that id, no matter if it's also present somewhere else
NNModelDesc* NNModelDescLib::load(char const* path)
{
    auto existingId = findId(path);
    unsigned short id = existingId < 65535 ? existingId : getNextId();
    return load(id, path);
}
NNModelDesc* NNModelDescLib::load(unsigned short id, char const* path)
{
    auto model = get(id, false);
    /* Print("NNBackend: loading model %s at idx %d\n", path, id); */
    if (model != nullptr)
    {
        if (strcmp(model->getPath(), path) == 0)
        {
            Print("NNBackend: model %d already loaded %s\n", id, path);
            return model;
        }
        else
        {
            return model->load(path) ? model : nullptr;
        }
    }

    model = new NNModelDesc(id);
    if (model->load(path))
    {
        models[id] = model;
        modelCount++;
        return model;
    }
    else
    {
        delete model;
        return nullptr;
    }
}

void NNModelDescLib::unload(unsigned short id)
{
    auto model = get(id, true);
    if (model == nullptr)
        return;
    /* Print("NNBackend: unloading model %s at idx %d\n", model->m_path, id); */
    models.erase(id);
    delete model;
}

bool NNModelDescLib::dumpAllInfo(char const* filename) const
{
    try
    {
        std::ofstream file;
        file.open(filename);
        if (!file.is_open())
        {
            Print("ERROR: NNBackend couldn't open file %s\n", filename);
            return false;
        }
        streamAllInfo(file);
        file.close();
        return true;
    }
    catch (...)
    {
        Print("ERROR: NNBackend couldn't dump info to file %s\n", filename);
        return false;
    }
}

// file dumps are needed to share info with client

void NNModelDesc::streamInfo(std::ostream& stream) const
{
    stream << "- idx: " << m_idx << "\n  modelPath: " << m_path.c_str()
           << "\n  minBufferSize: " << m_higherRatio << "\n  methods:";
    for (auto const& m : m_methods)
    {
        stream << "\n    - name: " << m.name << "\n      inDim: " << m.inDim
               << "\n      inRatio: " << m.inRatio << "\n      outDim: " << m.outDim
               << "\n      outRatio: " << m.outRatio;
    }
    if (m_attributes.size() > 0)
    {
        stream << "\n  attributes:";
        for (auto const& attr : m_attributes)
            stream << "\n    - " << attr.name;
    }
    stream << "\n";
}

void NNModelDesc::printInfo() const
{
    streamInfo(std::cout);
    std::cout << std::endl;
}

bool NNModelDesc::dumpInfo(char const* filename) const
{
    try
    {
        std::ofstream file;
        file.open(filename);
        if (!file.is_open())
        {
            Print("ERROR: NNBackend couldn't open file %s\n", filename);
            return false;
        }
        streamInfo(file);
        file.close();
        return true;
    }
    catch (...)
    {
        Print("ERROR: NNBackend couldn't dump info to file %s\n", filename);
        return false;
    }
}

} // namespace NN
