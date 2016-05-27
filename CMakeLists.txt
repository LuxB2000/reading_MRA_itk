cmake_minimum_required(VERSION 2.6)

PROJECT(ReadingMRA)

# optional: set your ITK directory
SET(ITK_DIR "/mine/InsightToolkit-4.8.1-build")

FIND_PACKAGE ( ITK REQUIRED )
IF (  ITK_FOUND  )
	INCLUDE(  ${ITK_USE_FILE}  )
ELSE(ITK_FOUND)
  MESSAGE(FATAL_ERROR "ITK not found. Please set ITK_DIR :-)")
ENDIF(   ITK_FOUND   )

