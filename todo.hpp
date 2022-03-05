#pragma once
template<typename R = void>
R todo() { return std::terminate(), R(); }