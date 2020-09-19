#ifndef __SVHER_SINGLETON_H_
#define __SVHER_SINGLETON_H_

#include <memory>

namespace svher {
    template <class T, class X = void, int N = 0>
    class Singleton {
    public:
        static T* GetInstance() {
            static T v;
            return &v;
        }
    };

    template<class T, class X = void, int N = 0>
    class SingletonPtr {
    public:
        static std::shared_ptr<T> GetInstance() {
            static std::shared_ptr<T> v(new T);
            return v;
        }
    };
}

#endif