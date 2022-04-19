.PHONY: init docs clean stop tests release tup-monitor watch watch-docs watch-browser-sync

name = game

init:
	@pip install --user mkdocs mkdocs-bootswatch pymdown-extensions entangled-filters mkdocs-kroki-plugin pygments; \
	tup variant builds/*.config; \
	cd third-party/luajit && make;

docs:
	mkdocs build

clean-executables:
	rm -f $(name) $(name)-tests $(name)-debug $(name)-release $(name)-debug-with-asan

clean: clean-executables
	cd third-party/luajit && make clean; \
	rm -rf docs; \
	rm -rf  build-*

watch:
	@tmux new-session make --no-print-directory watch-docs \; \
		split-window -v make --no-print-directory entangled \; \
		select-layout even-vertical \; \
		rename-window "Watchers" \; \
		new-window make --no-print-directory watch-tests \; \
		split-window -h make --no-print-directory tup-monitor \; \
		rename-window "Build & Test" \;

watch-docs:
	mkdocs serve

watch-tests:
	@while true; do \
		inotifywait -e close_write $(name)-tests ; \
		while [ ! -f $(name)-tests ]; do sleep 0.5; done; \
		./$(name)-tests; \
		echo ""; \
		sleep 1; \
	done

test: target = tests
test: build

tup-monitor:
	tup monitor -f -a build-tests

entangled:
	entangled daemon

stop:
	cd target && tup stop

ifeq ($(target),release)
build: executable = $(name)
else
build: executable = $(name)-$(target)
endif
build:
	tup build-$(target)

release:
	tup generate --config build-release/tup.config compile-release.sh

docker:
	docker build --rm -t $(name) .