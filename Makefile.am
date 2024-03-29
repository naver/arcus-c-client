ACLOCAL_AMFLAGS = -I m4

# includes append to these:
SUFFIXES =
.PHONY =
TESTS =
CLEANFILES =
DISTCLEANFILES =
bin_PROGRAMS =
noinst_HEADERS =
lib_LTLIBRARIES =
man_MANS =
noinst_LTLIBRARIES =
noinst_PROGRAMS =
include_HEADERS =
nobase_include_HEADERS =
check_PROGRAMS =
EXTRA_HEADERS =
BUILT_SOURCES=
EXTRA_DIST= \
	    ${srcdir}/m4/pandora_*.m4 \
	    .quickly \
	    config/autorun.sh \
	    config/pandora-plugin \
	    config/uncrustify.cfg

libmemcached_libmemcached_la_LDFLAGS=

include clients/include.am
include docs/include.am
include example/include.am
include libhashkit/include.am
include libmemcached/include.am
include libmemcached/util/include.am
include libmemcached/protocol/include.am
include libmemcachedinternal/include.am
include libmemcachedinternal/util/include.am
include libtest/include.am
include poll/include.am
include support/include.am
include tests/include.am
include util/include.am
include win32/include.am
if BUILD_ARCUS_ZK_INTEGRATION
# Samples that use arcus cluster API
include arcus/include.am
endif

docs: html man

TESTS += ${check_PROGRAMS}

EXTRA_DIST+= ${man_MANS}

fedora:
	@rm -f ~/rpmbuild/RPMS/x86_64/libmemcached-$(VERSION)*.rpm
	@rm -f ~/rpmbuild/SRPMS/libmemcached-$(VERSION)*.rpm
	@cp libmemcached-$(VERSION).tar.gz ~/rpmbuild/SOURCES/
	@rpmbuild -ba support/libmemcached.spec
	@cp ~/rpmbuild/RPMS/x86_64/libmemcached-$(VERSION)*.rpm .
	@cp ~/rpmbuild/SRPMS/libmemcached-$(VERSION)*.rpm .

generic: support/libmemcached.spec
	@rm -f ~/rpmbuild/RPMS/x86_64/libmemcached-$(VERSION)*.rpm
	@rm -f ~/rpmbuild/SRPMS/libmemcached-$(VERSION)*.rpm
	@cp libmemcached-$(VERSION).tar.gz ~/rpmbuild/SOURCES/
	@rpmbuild -ba support/libmemcached.spec
	@cp ~/rpmbuild/RPMS/x86_64/libmemcached-$(VERSION)*.rpm .
	@cp ~/rpmbuild/RPMS/x86_64/libmemcached-devel-$(VERSION)*.rpm .
	@cp ~/rpmbuild/RPMS/x86_64/libmemcached-debuginfo-$(VERSION)*.rpm .
	@cp ~/rpmbuild/SRPMS/libmemcached-$(VERSION)*.rpm .

rpm: man distcheck generic

release: rpm
	@rpm --addsign libmemcached-$(VERSION)*.rpm .
	@rpm --checksig libmemcached-$(VERSION)*.rpm .
	@rpm --addsign libmemcached-devel-$(VERSION)*.rpm .
	@rpm --checksig libmemcached-devel-$(VERSION)*.rpm .
	@rpm --addsign libmemcached-debuginfo-$(VERSION)*.rpm .
	@rpm --checksig libmemcached-debuginfo-$(VERSION)*.rpm .

merge-clean:
	@find ./ | $(GREP) \.gcda | xargs rm -f
	@find ./ | $(GREP) \.gcno | xargs rm -f
	@find ./ | $(GREP) \.gz | xargs rm -f
	@find ./ | $(GREP) \.orig | xargs rm -f
	@find ./ | $(GREP) \.rej | xargs rm -f
	@find ./ | $(GREP) \.rpm | xargs rm -f
	@find ./ | $(GREP) \.THIS | xargs rm -f
	@find ./ | $(GREP) \.OTHER | xargs rm -f
	@find ./ | $(GREP) \.BASE | xargs rm -f
	@find ./ | $(GREP) \~$$ | xargs rm -f
	@echo "Files that need to be either removed or checked in:"
	@bzr unknowns

clean-local:
	-rm -rf docs/_build docs/doctrees

lcov: lcov-clean check
	@echo
	@echo "	------------------------------------------------------"
	@echo "Make sure ./configure was run with '--enable-coverage'"
	@echo "------------------------------------------------------"
	@echo
	@cd libmemcached && lcov --capture --directory . --base-directory .. --output-file lcov.out
	@genhtml -o lcov --function-coverage -t libmemcached libmemcached/lcov.out

lcov-clean: clean
	@rm -rf lcov */lcov.out
	@find . -name '*.gcno' | xargs rm -f
	@find . -name '*.gcda' | xargs rm -f

DISTCLEANFILES+= config/top.h
