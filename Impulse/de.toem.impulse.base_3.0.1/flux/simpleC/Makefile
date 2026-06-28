
DIRS = examples
BUILD = $(DIRS)
TRACE = $(DIRS)
all:
	for d in $(BUILD); do (cd $$d && $(MAKE) all ); done       
	./build/build  
	        
clean:
	for d in $(DIRS); do (cd $$d && $(MAKE) clean ); done
	rm -rf doc
	
trace:
	for d in $(TRACE); do (cd $$d && $(MAKE) trace ); done