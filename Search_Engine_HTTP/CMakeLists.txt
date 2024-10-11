cmake_minimum_required(VERSION 3.22.0)

set (ICU_ROOT C:/kurs_cpp/vcpkg/packages/icu_x64-windows/)
set (BOOST_ROOT C:/boost-1.85.0/)
set (OPENSSL_ROOT_DIR C:/kurs_cpp/OpenSSL/openssl-1.1/x64)
 
set (CMAKE_CXX_STANDARD 17) 

set (Boost_NO_SYSTEM_PATHS ON)
set (Boost_USE_MULTITHREADED ON)
set (Boost_USE_STATIC_LIBS ON)
set (Boost_USE_STATIC_RUNTIME OFF)
set (BOOST_ALL_DYN_LINK OFF)

set(ICU_USE_STATIC_LIBS ON)
set(OPENSSL_USE_STATIC_LIBS ON)

project (Search_Engine_HTTP)

find_package(ICU COMPONENTS uc i18n REQUIRED)
  
find_package (Boost REQUIRED ) 

#set(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)

find_package(OpenSSL REQUIRED)

#-----------------------------------------------------
if (OPENSSL_FOUND)

#    include_directories (Boost ${Icu_INCLUDE_DIR})
    include_directories (Boost ${Boost_INCLUDE_DIR})


    add_executable (Search_Engine_HTTP
        main.cpp
#        ${ICU_BEAST_FILES}
        ${BOOST_BEAST_FILES}
        root_certificates.h
        config.ini
        indexer.h
        indexer.cpp
        spider_client.h
        spider_client.cpp 
        spider_read_config.h 
        server.h 
        )

     add_subdirectory(C:/kurs_cpp/libpqxx-7.7.4 libpqxx-build) 

    target_link_libraries(Search_Engine_HTTP ICU::uc ICU::i18n)

    target_link_libraries (Search_Engine_HTTP #${ICU_LIBRARIES}
        pqxx
        ${Boost_LIBRARIES} 
        OpenSSL::SSL 
        OpenSSL::Crypto
        )

endif()
#---------------------------------------------
