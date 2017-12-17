# APPLY_LOCALE_BLACKLIST is a CMake option - can be defined externally
# BUILD_CLIENT - if the build will be creating the ${PROJECT_NAME} executable - can be defined externally

macro(GETTEXT_HELPER ENABLE BLACKLISTED_LOCALES)

	find_package(GettextLib)

	if(ENABLE AND GETTEXT_FOUND)
		message(STATUS "GetText library: ${GETTEXT_LIBRARY}")
		message(STATUS "GetText DLL: ${GETTEXT_DLL}")
		message(STATUS "GetText iconv DLL: ${GETTEXT_ICONV_DLL}")
		message(STATUS "GetText enabled; locales found: ${GETTEXT_AVAILABLE_LOCALES}")

		# decide which locales belong into GETTEXT_USED_LOCALES.
		#   find_package(GettextLib) gave us GETTEXT_AVAILABLE_LOCALES.
		#   if APPLY_LOCALE_BLACKLIST is specified we filter available locales
		#   according to the GETTEXT_BLACKLISTED_LOCALES list.
		
		if (GETTEXT_FOUND AND APPLY_LOCALE_BLACKLIST)
			set(GETTEXT_USED_LOCALES "")
			foreach(LOCALE ${GETTEXT_AVAILABLE_LOCALES})
				if (NOT ";${GETTEXT_BLACKLISTED_LOCALES};" MATCHES ";${LOCALE};")
					list(APPEND GETTEXT_USED_LOCALES ${LOCALE})
				endif()
			endforeach()
			message(STATUS "Locale blacklist applied; Locales used: ${GETTEXT_USED_LOCALES}")
		endif()
		
		if (BUILD_CLIENT)
			if(ENABLE AND GETTEXT_FOUND)
				foreach(LOCALE ${GETTEXT_USED_LOCALES})
					set_mo_paths(MO_BUILD_PATH MO_DEST_PATH ${LOCALE})
					set(MO_BUILD_PATH "${MO_BUILD_PATH}/${PROJECT_NAME}.mo")
					install(FILES ${MO_BUILD_PATH} DESTINATION ${MO_DEST_PATH})
				endforeach()
			endif()
		endif()
		
		if(BUILD_CLIENT)
			if(GETTEXT_DLL)
				install(FILES ${GETTEXT_DLL} DESTINATION ${BINDIR})
			endif()
			if(GETTEXT_ICONV_DLL)
				install(FILES ${GETTEXT_ICONV_DLL} DESTINATION ${BINDIR})
			endif()
		endif()

		if(TRUE)
			set(MO_FILES)

			foreach(LOCALE ${GETTEXT_USED_LOCALES})
				set(PO_FILE_PATH "${GETTEXT_PO_PATH}/${LOCALE}/${PROJECT_NAME}.po")
				set_mo_paths(MO_BUILD_PATH MO_DEST_PATH ${LOCALE})
				set(MO_FILE_PATH "${MO_BUILD_PATH}/${PROJECT_NAME}.mo")

				add_custom_command(OUTPUT ${MO_BUILD_PATH}
					COMMAND ${CMAKE_COMMAND} -E make_directory ${MO_BUILD_PATH}
					COMMENT "mo-update [${LOCALE}]: Creating locale directory.")

				add_custom_command(
					OUTPUT ${MO_FILE_PATH}
					COMMAND ${GETTEXT_MSGFMT} -o ${MO_FILE_PATH} ${PO_FILE_PATH}
					DEPENDS ${MO_BUILD_PATH} ${PO_FILE_PATH}
					WORKING_DIRECTORY "${GETTEXT_PO_PATH}/${LOCALE}"
					COMMENT "mo-update [${LOCALE}]: Creating mo file."
					)

				set(MO_FILES ${MO_FILES} ${MO_FILE_PATH})
			endforeach()

			add_custom_target(translations ALL COMMENT "mo update" DEPENDS ${MO_FILES})
		endif()
		
	endif()

endmacro()
