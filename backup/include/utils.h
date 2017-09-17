#pragma once

/**
 * @brief Disable copy macro
 */
#define DISABLE_COPY(CLASS_NAME)                            \
    CLASS_NAME(CLASS_NAME &&) = delete;                     \
    CLASS_NAME(const CLASS_NAME &other) = delete;           \
    CLASS_NAME& operator=(const CLASS_NAME &other) = delete
    