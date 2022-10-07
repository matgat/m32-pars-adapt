#ifndef GUARD_vectset_hpp
#define GUARD_vectset_hpp
//  ---------------------------------------------
//  A set implemented with vector
//  ---------------------------------------------
#include <vector>
#include <type_traits> // std::is_same_v, std::decay_t
//#include <concepts> // std::same_as

//:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
namespace mat //:::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::
{

/////////////////////////////////////////////////////////////////////////////
template<class elem_type> class vectset
{
 public:
    using container_type = std::vector<elem_type>;
    using iterator = container_type::iterator;
    using const_iterator = container_type::const_iterator;


    [[nodiscard]] auto size() const noexcept { return vect.size(); }
    [[nodiscard]] bool is_empty() const noexcept { return vect.empty(); }
    void clear() { vect.clear(); }


    //void insert(const elem_type& new_elem)
    //   {
    //    if( !contains(new_elem) )
    //       {
    //        vect.push_back(new_elem);
    //       }
    //   }
    template<class T> //template<std::same_as<elem_type> std::decay_t<T>>
    void insert(T&& new_elem)
       {
        static_assert(std::is_same_v<elem_type, std::decay_t<T>>, "Cannot insert this type in vectset");
        if( !contains(new_elem) )
           {
            vect.push_back(std::forward<T>(new_elem));
           }
       }

    //template<class... Args> void emplace(Args&&... args)
    //   {
    //    vect.emplace_back(std::forward<Args>(args)...);
    //   }


    [[maybe_unused]] bool erase(const elem_type& elem)
       {
        iterator ielem = vect.begin();
        while( ielem!=vect.end() )
           {
            if( *ielem==elem )
               {
                ielem = vect.erase(ielem);
                return true;
               }
            else ++ielem;
           }
        return false;
       }

    [[nodiscard]] bool contains(const elem_type& other_elem) const noexcept
       {
        for(const elem_type& elem : vect)
           {
            if(elem==other_elem) return true;
           }
        return false;
       }


    [[nodiscard]] bool contains(const vectset& other) const noexcept
       {
        for( const elem_type& other_elem : other.vect )
           {
            if( !contains(other_elem) ) return false;
           }
        return true;
       }

    void merge(const vectset& other)
       {
        for( const elem_type& other_elem : other.vect )
           {
            insert(other_elem);
           }
       }


    [[nodiscard]] const_iterator begin() const noexcept { return vect.cbegin(); }
    [[nodiscard]] const_iterator end() const noexcept { return vect.cend(); }
    [[nodiscard]] iterator begin() noexcept { return vect.begin(); }
    [[nodiscard]] iterator end() noexcept { return vect.end(); }

 private:
    container_type vect;

    //iterator find(const elem_type& elem)
    //   {
    //    for( iterator ielem=vect.begin(); ielem!=vect.end(); ++ielem )
    //       {
    //        if( *ielem==elem ) return ielem;
    //       }
    //    return ielem;
    //   }
};


}//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::


//---- end unit -------------------------------------------------------------
#endif
