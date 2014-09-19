include(FindGettext REQUIRED)
find_program(GETTEXT_XGETTEXT_EXECUTABLE xgettext)

set(DOMAIN trust-store)
set(POT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/${DOMAIN}.pot)
file(GLOB PO_FILES *.po)

# We look through all source files and find
# usages of i18n::tr.
file(
    GLOB_RECURSE I18N_CPP_SRC_FILES
    ${CMAKE_SOURCE_DIR}/src/*.cpp ${CMAKE_SOURCE_DIR}/src/*.qml)

list(SORT I18N_CPP_SRC_FILES)

add_custom_target(
    trust-store-pot
    COMMAND ${GETTEXT_XGETTEXT_EXECUTABLE}
        --default-domain=trust-store
        --output=${POT_FILE}
        --from-code=UTF-8
        --c++ --add-comments=TRANSLATORS
        --keyword='tr'
        --boost
        --package-name=trust-store
        --copyright-holder='Canonical Ltd.'
        ${I18N_CPP_SRC_FILES})

foreach(PO_FILE ${PO_FILES})
    get_filename_component(LANG ${PO_FILE} NAME_WE)
    gettext_process_po_files(${LANG} ALL PO_FILES ${PO_FILE})
    set(INSTALL_DIR ${CMAKE_INSTALL_LOCALEDIR}/${LANG}/LC_MESSAGES)
    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/${LANG}.gmo
        DESTINATION ${INSTALL_DIR}
        RENAME ${DOMAIN}.mo)
endforeach(PO_FILE)
