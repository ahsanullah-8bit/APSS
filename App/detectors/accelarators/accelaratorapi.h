#pragma once

#include <memory>
#include <QString>

#include "config/predictorconfig.h"
#include "apss.h"

/*
    NOTICE: For the Accelarator API to work, you must register AFTER the subclass
        definition using the macro REGISTER_ACCELARATOR. It will create
*/
class AccelaratorApi {
public:
    using FactoryFunction = std::unique_ptr<AccelaratorApi>(*)(const AccelaratorConfigVariant& config);

    explicit AccelaratorApi(const AccelaratorConfigVariant& config);
    virtual ~AccelaratorApi();
    virtual std::vector<PredictionList> predict() = 0;
    const AccelaratorConfigVariant& config() const;

private:
    AccelaratorConfigVariant m_config;
};

// // -----
// #define REGISTER_ACCELARATOR(DerivedClass, Key)                                                         \
//     static std::unique_ptr<AccelaratorApi> create##DerivedClass(const AccelaratorConfigVariant& config) {   \
//         return std::make_unique<AccelaratorApi>(config);                                                    \
//     }                                                                                                       \
//     inline static bool registered##DerivedClass = registerType(Key, create##DerivedClass);

// Definitions
inline AccelaratorApi::AccelaratorApi(const AccelaratorConfigVariant& config)
    : m_config(config)
{}

inline AccelaratorApi::~AccelaratorApi()
{}
