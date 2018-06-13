function cleanup()
	log("Unregistering CPU")
	depth(depth() + 1)
	unregisterCPU(Z80)
end
