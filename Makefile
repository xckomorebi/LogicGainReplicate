PROJECT_FILE=Builds/MacOSX/LogicGain.xcodeproj

all:
	xcodebuild -project ${PROJECT_FILE} -scheme "LogicGain - AU" -configuration Debug

clean:
	xcodebuild -project ${PROJECT_FILE} clean
	rm -r ${HOME}/Library/Audio/Plug-Ins/Components/LogicGain.component