#pragma once
template <typename T, template <typename, typename...> class V>
    class vec_order_comp {
        std::map<T, std::size_t> ordmap;

        public:
        vec_order_comp(V<T> const& vec) {
            std::size_t i = 0;
            for (auto it = vec.begin(); it != vec.end(); it++) {
                ordmap[*it] = i;
            }
        }

        bool operator()(T v1, T v2) {
            return ordmap[v1] < ordmap[v2];
        }
    };
    