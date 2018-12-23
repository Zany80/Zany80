#pragma once

#include <Zany80/Plugin.h>

class scas : public ToolchainPlugin {
	OryolClassDecl(scas);
	OryolTypeDecl(scas, ToolchainPlugin);
public:
	virtual int transform(Array<String> sources, String destination, StringBuilder *out);
	virtual Map<String, Array<String>> supportedTransforms();
	virtual bool supports(String type);
	virtual void addIncludeDirectory(String dir);
	virtual String getChain();
	virtual void setVerbosity(int verbosity);
private:
	StringBuilder include_dirs;
};
