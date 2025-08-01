// -*- C++ -*-
//
// This file was generated by ODB, object-relational mapping (ORM)
// compiler for C++.
//

#ifndef EXPORTS_ODB_HXX
#define EXPORTS_ODB_HXX

// Begin prologue.
//
#include <odb/qt/version.hxx>
#if ODB_QT_VERSION != 2050000 // 2.5.0
#  error ODB and C++ compilers see different libodb-qt interface versions
#endif
#include <odb/qt/basic/sqlite/qstring-traits.hxx>
#include <odb/qt/basic/sqlite/qbyte-array-traits.hxx>
#include <odb/qt/basic/sqlite/quuid-traits.hxx>
#include <odb/qt/containers/qhash-traits.hxx>
#include <odb/qt/containers/qlist-traits.hxx>
#include <odb/qt/containers/qlinked-list-traits.hxx>
#include <odb/qt/containers/qmap-traits.hxx>
#include <odb/qt/containers/qset-traits.hxx>
#include <odb/qt/containers/qvector-traits.hxx>
#include <odb/qt/date-time/sqlite/qdate-traits.hxx>
#include <odb/qt/date-time/sqlite/qtime-traits.hxx>
#include <odb/qt/date-time/sqlite/qdate-time-traits.hxx>
#include <QtCore/QSharedPointer>
#include <odb/qt/smart-ptr/pointer-traits.hxx>
#include <odb/qt/smart-ptr/wrapper-traits.hxx>
//
// End prologue.

#include <odb/version.hxx>

#if ODB_VERSION != 20500UL
#error ODB runtime version mismatch
#endif

#include <odb/pre.hxx>

#include "exports.h"

#include <memory>
#include <cstddef>
#include <utility>

#include <odb/core.hxx>
#include <odb/traits.hxx>
#include <odb/callback.hxx>
#include <odb/wrapper-traits.hxx>
#include <odb/pointer-traits.hxx>
#include <odb/container-traits.hxx>
#include <odb/no-op-cache-traits.hxx>
#include <odb/result.hxx>
#include <odb/simple-object-result.hxx>

#include <odb/details/unused.hxx>
#include <odb/details/shared-ptr.hxx>

namespace odb
{
  // Exports
  //
  template <>
  struct class_traits< ::Exports >
  {
    static const class_kind kind = class_object;
  };

  template <>
  class access::object_traits< ::Exports >
  {
    public:
    typedef ::Exports object_type;
    typedef ::QSharedPointer< ::Exports > pointer_type;
    typedef odb::pointer_traits<pointer_type> pointer_traits;

    static const bool polymorphic = false;

    typedef ::QString id_type;

    static const bool auto_id = false;

    static const bool abstract = false;

    static id_type
    id (const object_type&);

    typedef
    no_op_pointer_cache_traits<pointer_type>
    pointer_cache_traits;

    typedef
    no_op_reference_cache_traits<object_type>
    reference_cache_traits;

    static void
    callback (database&, object_type&, callback_event);

    static void
    callback (database&, const object_type&, callback_event);
  };
}

#include <odb/details/buffer.hxx>

#include <odb/sqlite/version.hxx>
#include <odb/sqlite/forward.hxx>
#include <odb/sqlite/binding.hxx>
#include <odb/sqlite/sqlite-types.hxx>
#include <odb/sqlite/query.hxx>

namespace odb
{
  // Exports
  //
  template <typename A>
  struct query_columns< ::Exports, id_sqlite, A >
  {
    // id
    //
    typedef
    sqlite::query_column<
      sqlite::value_traits<
        ::QString,
        sqlite::id_text >::query_type,
      sqlite::id_text >
    id_type_;

    static const id_type_ id;

    // camera
    //
    typedef
    sqlite::query_column<
      sqlite::value_traits<
        ::QString,
        sqlite::id_text >::query_type,
      sqlite::id_text >
    camera_type_;

    static const camera_type_ camera;

    // name
    //
    typedef
    sqlite::query_column<
      sqlite::value_traits<
        ::QString,
        sqlite::id_text >::query_type,
      sqlite::id_text >
    name_type_;

    static const name_type_ name;

    // date
    //
    typedef
    sqlite::query_column<
      sqlite::value_traits<
        ::QDateTime,
        sqlite::id_text >::query_type,
      sqlite::id_text >
    date_type_;

    static const date_type_ date;

    // videoPath
    //
    typedef
    sqlite::query_column<
      sqlite::value_traits<
        ::QString,
        sqlite::id_text >::query_type,
      sqlite::id_text >
    videoPath_type_;

    static const videoPath_type_ videoPath;

    // thumbPath
    //
    typedef
    sqlite::query_column<
      sqlite::value_traits<
        ::QString,
        sqlite::id_text >::query_type,
      sqlite::id_text >
    thumbPath_type_;

    static const thumbPath_type_ thumbPath;

    // inProgress
    //
    typedef
    sqlite::query_column<
      sqlite::value_traits<
        bool,
        sqlite::id_integer >::query_type,
      sqlite::id_integer >
    inProgress_type_;

    static const inProgress_type_ inProgress;
  };

  template <typename A>
  const typename query_columns< ::Exports, id_sqlite, A >::id_type_
  query_columns< ::Exports, id_sqlite, A >::
  id (A::table_name, "\"id\"", 0);

  template <typename A>
  const typename query_columns< ::Exports, id_sqlite, A >::camera_type_
  query_columns< ::Exports, id_sqlite, A >::
  camera (A::table_name, "\"camera\"", 0);

  template <typename A>
  const typename query_columns< ::Exports, id_sqlite, A >::name_type_
  query_columns< ::Exports, id_sqlite, A >::
  name (A::table_name, "\"name\"", 0);

  template <typename A>
  const typename query_columns< ::Exports, id_sqlite, A >::date_type_
  query_columns< ::Exports, id_sqlite, A >::
  date (A::table_name, "\"date\"", 0);

  template <typename A>
  const typename query_columns< ::Exports, id_sqlite, A >::videoPath_type_
  query_columns< ::Exports, id_sqlite, A >::
  videoPath (A::table_name, "\"videoPath\"", 0);

  template <typename A>
  const typename query_columns< ::Exports, id_sqlite, A >::thumbPath_type_
  query_columns< ::Exports, id_sqlite, A >::
  thumbPath (A::table_name, "\"thumbPath\"", 0);

  template <typename A>
  const typename query_columns< ::Exports, id_sqlite, A >::inProgress_type_
  query_columns< ::Exports, id_sqlite, A >::
  inProgress (A::table_name, "\"inProgress\"", 0);

  template <typename A>
  struct pointer_query_columns< ::Exports, id_sqlite, A >:
    query_columns< ::Exports, id_sqlite, A >
  {
  };

  template <>
  class access::object_traits_impl< ::Exports, id_sqlite >:
    public access::object_traits< ::Exports >
  {
    public:
    struct id_image_type
    {
      details::buffer id_value;
      std::size_t id_size;
      bool id_null;

      std::size_t version;
    };

    struct image_type
    {
      // m_id
      //
      details::buffer m_id_value;
      std::size_t m_id_size;
      bool m_id_null;

      // m_camera
      //
      details::buffer m_camera_value;
      std::size_t m_camera_size;
      bool m_camera_null;

      // m_name
      //
      details::buffer m_name_value;
      std::size_t m_name_size;
      bool m_name_null;

      // m_date
      //
      details::buffer m_date_value;
      std::size_t m_date_size;
      bool m_date_null;

      // m_videoPath
      //
      details::buffer m_videoPath_value;
      std::size_t m_videoPath_size;
      bool m_videoPath_null;

      // m_thumbPath
      //
      details::buffer m_thumbPath_value;
      std::size_t m_thumbPath_size;
      bool m_thumbPath_null;

      // m_inProgress
      //
      long long m_inProgress_value;
      bool m_inProgress_null;

      std::size_t version;
    };

    struct extra_statement_cache_type;

    using object_traits<object_type>::id;

    static id_type
    id (const image_type&);

    static bool
    grow (image_type&,
          bool*);

    static void
    bind (sqlite::bind*,
          image_type&,
          sqlite::statement_kind);

    static void
    bind (sqlite::bind*, id_image_type&);

    static bool
    init (image_type&,
          const object_type&,
          sqlite::statement_kind);

    static void
    init (object_type&,
          const image_type&,
          database*);

    static void
    init (id_image_type&, const id_type&);

    typedef sqlite::object_statements<object_type> statements_type;

    typedef sqlite::query_base query_base_type;

    static const std::size_t column_count = 7UL;
    static const std::size_t id_column_count = 1UL;
    static const std::size_t inverse_column_count = 0UL;
    static const std::size_t readonly_column_count = 0UL;
    static const std::size_t managed_optimistic_column_count = 0UL;

    static const std::size_t separate_load_column_count = 0UL;
    static const std::size_t separate_update_column_count = 0UL;

    static const bool versioned = false;

    static const char persist_statement[];
    static const char find_statement[];
    static const char update_statement[];
    static const char erase_statement[];
    static const char query_statement[];
    static const char erase_query_statement[];

    static const char table_name[];

    static void
    persist (database&, const object_type&);

    static pointer_type
    find (database&, const id_type&);

    static bool
    find (database&, const id_type&, object_type&);

    static bool
    reload (database&, object_type&);

    static void
    update (database&, const object_type&);

    static void
    erase (database&, const id_type&);

    static void
    erase (database&, const object_type&);

    static result<object_type>
    query (database&, const query_base_type&);

    static unsigned long long
    erase_query (database&, const query_base_type&);

    public:
    static bool
    find_ (statements_type&,
           const id_type*);

    static void
    load_ (statements_type&,
           object_type&,
           bool reload);
  };

  template <>
  class access::object_traits_impl< ::Exports, id_common >:
    public access::object_traits_impl< ::Exports, id_sqlite >
  {
  };

  // Exports
  //
}

#include "exports-odb.ixx"

#include <odb/post.hxx>

#endif // EXPORTS_ODB_HXX
