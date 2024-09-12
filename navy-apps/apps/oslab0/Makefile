REPO_PATH = repo
ifeq ($(wildcard repo/161220016/main.c),)
  $(shell git clone --depth=1 git@github.com:NJU-ProjectN/oslab0-collection.git $(REPO_PATH))
endif

GAMES = $(shell ls ./repo | grep -E "[0-9]{9}")

# ALL = $(GAMES)
#     推箱子     Blendoku  躲避方块   俄罗斯方块  七色版2048 猜词小游戏  雷电
ALL = 171240502 171240518 171860013 171860508 171860532 171860575 171860637
ifeq ($(filter $(GAMES), $(ALL)),)
  $(error Expected games in {$(GAMES)}, Got "$(ALL)")
endif

ALL_PATH = $(foreach app,$(ALL),./repo/$(app))

ifeq ($(findstring $(MAKECMDGOALS),clean|clean-all|html),)
libam:
	$(MAKE) -C $(NAVY_HOME)/libs/libam archive
else
libam: ;
endif

default $(MAKECMDGOALS): libam
	-for f in $(ALL_PATH); do AM_HOME=$(NAVY_HOME) LIBS=libam $(MAKE) -C $$f -f ../Makefile $(MAKECMDGOALS); done

.PHONY: libam default $(MAKECMDGOALS)

init:;
.PHONY: init
