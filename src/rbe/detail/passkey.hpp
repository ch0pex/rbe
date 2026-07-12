/************************************************************************
 * Copyright (c) 2026 Alvaro Cabrera Barrio
 * This code is licensed under MIT license (see LICENSE.txt for details)
 ************************************************************************/
/**
 * @file passkey.hpp
 * @date 26/05/2026
 * @brief PassKey idiom for controlled access to private constructors
 */

#pragma once

// --- Includes ---

// --- Dependencies ---

// --- External dependencies ---

// --- STD ---

// --- System ---


namespace mono {

/**
 * PassKey class can be used as key for those restricted constructors
 * this is very handy when you want to restrict the creating of an object
 */
template<typename T>
class passkey {
  friend T;
  explicit passkey()      = default;
  passkey(passkey const&) = default;
  ~passkey()              = default;

public:
  passkey(passkey&&)                 = delete;
  passkey& operator=(passkey const&) = delete;
  passkey& operator=(passkey&&)      = delete;
};

} // namespace mono
