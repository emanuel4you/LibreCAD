TEMPLATE = lib

CONFIG += static warn_on

DESTDIR = ../../generated/lib

VERSION = 0.12.1

DLL_NAME = slide
TARGET = $$DLL_NAME

GENERATED_DIR = ../../generated/lib/libslide
# Use common project definitions.
include(../../common.pri)

# svg support
QT -= svg

SOURCES += \
    src/slide_draw_qpainter.cpp \
    src/slide_binary_parser.cpp \
    src/slide_binary_writer.cpp \
    src/slide_cache.cpp \
    src/slide_colors.cpp \
    src/slide.cpp \
    src/slide_endian.cpp \
    src/slide_header_binary_parser.cpp \
    src/slide_header_binary_writer.cpp \
    src/slide_info_text_writer.cpp \
    src/slide_library_binary_parser.cpp \
    src/slide_library_binary_writer.cpp \
    src/slide_library.cpp \
    src/slide_library_directory_binary_parser.cpp \
    src/slide_library_directory_binary_writer.cpp \
    src/slide_library_header_binary_parser.cpp \
    src/slide_library_header_binary_writer.cpp \
    src/slide_library_info_text_writer.cpp \
    src/slide_loader.cpp \
    src/slide_record_binary_parser.cpp \
    src/slide_records_visitor_binary_writer.cpp \
    src/slide_records_visitor_qpainter_drawer.cpp \
    src/slide_records_visitor_stat.cpp \
    src/slide_records_visitor_text_writer.cpp \
    src/slide_record_text_parser.cpp \
    src/slide_util.cpp


HEADERS += \
    src/slide_draw_qpainter.h \
    src/slide_binary_parser.hpp \
    src/slide_binary_util.hpp \
    src/slide_binary_writer.hpp \
    src/slide_cache.hpp \
    src/slide_colors.hpp \
    src/slide_endian.hpp \
    src/slide_header_binary_parser.hpp \
    src/slide_header_binary_writer.hpp \
    src/slide_header.hpp \
    src/slide.hpp \
    src/slide_info_text_writer.hpp \
    src/slide_library_binary_parser.hpp \
    src/slide_library_binary_writer.hpp \
    src/slide_library_directory_binary_parser.hpp \
    src/slide_library_directory_binary_writer.hpp \
    src/slide_library_directory.h \
    src/slide_library_directory.hpp \
    src/slide_library_header_binary_parser.hpp \
    src/slide_library_header_binary_writer.hpp \
    src/slide_library_header.hpp \
    src/slide_library.hpp \
    src/slide_library_info_text_writer.hpp \
    src/slide_loader.hpp \
    src/slide_record_binary_parser.hpp \
    src/slide_records.hpp \
    src/slide_records_visitor_binary_writer.hpp \
    src/slide_records_visitor_qt_drawer.hpp \
    src/slide_records_visitor_qpainter_drawer.hpp \
    src/slide_records_visitor.hpp \
    src/slide_records_visitor_stat.hpp \
    src/slide_records_visitor_text_writer.hpp \
    src/slide_record_text_parser.hpp \
    src/slide_util.hpp \
    src/slide_version.hpp
