#!/bin/bash

# Copyright (c) 2019 Michael Jeanson <mjeanson@efficios.com>
# Copyright (C) 2019 Philippe Proulx <pproulx@efficios.com>
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; under version 2 of the License.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

# This file is meant to be sourced at the start of shell script-based tests.


# Error out when encountering an undefined variable
set -u

scriptdir="$(dirname "${BASH_SOURCE[0]}")"

# The OS on which we are running. See [1] for possible values of 'uname -s'.
# We do a bit of translation to ease our life down the road for comparison.
# Export it so that called executables can use it.
# [1] https://en.wikipedia.org/wiki/Uname#Examples
if [ "x${BT_OS_TYPE:-}" = "x" ]; then
	BT_OS_TYPE="$(uname -s)"
	case "$BT_OS_TYPE" in
	MINGW*)
		BT_OS_TYPE="mingw"
		;;
	Darwin)
		BT_OS_TYPE="darwin"
		;;
	Linux)
		BT_OS_TYPE="linux"
		;;
	*)
		BT_OS_TYPE="unsupported"
		;;
	esac
fi
export BT_OS_TYPE

# Allow overriding the source and build directories
if [ "x${BT_TESTS_SRCDIR:-}" = "x" ]; then
	BT_TESTS_SRCDIR="$scriptdir/.."
fi
export BT_TESTS_SRCDIR

if [ "x${BT_TESTS_BUILDDIR:-}" = "x" ]; then
	BT_TESTS_BUILDDIR="$scriptdir/.."
fi
export BT_TESTS_BUILDDIR

# By default, it will not source tap.sh.  If you to tap output directly from
# the test script, define the 'SH_TAP' variable to '1' before sourcing this
# script.
if [ "x${SH_TAP:-}" = x1 ]; then
	# shellcheck source=./tap/tap.sh
	. "${BT_TESTS_SRCDIR}/utils/tap/tap.sh"
fi

# Allow overriding the babeltrace2 executables
if [ "x${BT_TESTS_BT2_BIN:-}" = "x" ]; then
	BT_TESTS_BT2_BIN="$BT_TESTS_BUILDDIR/../src/cli/babeltrace2"
	if [ "$BT_OS_TYPE" = "mingw" ]; then
		BT_TESTS_BT2_BIN="${BT_TESTS_BT2_BIN}.exe"
	fi
fi
export BT_TESTS_BT2_BIN

if [ "x${BT_TESTS_BT2LOG_BIN:-}" = "x" ]; then
	BT_TESTS_BT2LOG_BIN="$BT_TESTS_BUILDDIR/../src/cli/babeltrace2-log"
	if [ "$BT_OS_TYPE" = "mingw" ]; then
		BT_TESTS_BT2LOG_BIN="${BT_TESTS_BT2LOG_BIN}.exe"
	fi
fi
export BT_TESTS_BT2LOG_BIN

# TODO: Remove when bindings/python/bt2/test_plugin.py is fixed
BT_PLUGINS_PATH="${BT_TESTS_BUILDDIR}/../src/plugins"

# Allow overriding the babeltrace2 plugin path
if [ "x${BT_TESTS_BABELTRACE_PLUGIN_PATH:-}" = "x" ]; then
	BT_TESTS_BABELTRACE_PLUGIN_PATH="${BT_PLUGINS_PATH}/ctf:${BT_PLUGINS_PATH}/utils:${BT_PLUGINS_PATH}/text"
fi

if [ "x${BT_TESTS_PROVIDER_DIR:-}" = "x" ]; then
	BT_TESTS_PROVIDER_DIR="${BT_TESTS_BUILDDIR}/../src/python-plugin-provider/.libs"
fi

# Allow overriding the babeltrace2 executables
if [ "x${BT_TESTS_PYTHONPATH:-}" = "x" ]; then
	BT_TESTS_PYTHONPATH="${BT_TESTS_BUILDDIR}/../src/bindings/python/bt2/build/build_lib"
fi


### External Tools ###
if [ "x${BT_TESTS_AWK_BIN:-}" = "x" ]; then
	BT_TESTS_AWK_BIN="awk"
fi
export BT_TESTS_AWK_BIN

if [ "x${BT_TESTS_GREP_BIN:-}" = "x" ]; then
	BT_TESTS_GREP_BIN="grep"
fi
export BT_TESTS_GREP_BIN

if [ "x${BT_TESTS_PYTHON_BIN:-}" = "x" ]; then
	BT_TESTS_PYTHON_BIN="python3"
fi
export BT_TESTS_PYTHON_BIN

if [ "x${BT_TESTS_SED_BIN:-}" = "x" ]; then
	BT_TESTS_SED_BIN="sed"
fi
export BT_TESTS_SED_BIN


# Data files path
BT_TESTS_DATADIR="${BT_TESTS_SRCDIR}/data"
BT_CTF_TRACES_PATH="${BT_TESTS_DATADIR}/ctf-traces"
BT_DEBUG_INFO_PATH="${BT_TESTS_DATADIR}/debug-info"


### Diff Functions ###

# Checks the difference between the content of the file with path "$1"
# and the output of the CLI when called with the rest of arguments
# to this function.
#
# Returns 0 if there's no difference, and 1 if there is, also printing
# said difference to the standard error.
bt_diff_cli() {
	local expected_file="$1"
	shift 1
	local args=("$@")

	local temp_output_file
	local temp_diff
	local ret=0

	temp_output_file="$(mktemp)"
	temp_diff="$(mktemp)"

	# Run the CLI to get a detailed file. Strip any \r present due to
	# Windows (\n -> \r\n). "diff --string-trailing-cr" is not used since it
	# is not present on Solaris.
	run_python_bt2 "$BT_TESTS_BT2_BIN" "${args[@]}" | tr -d "\r" > "$temp_output_file"

	# Compare output with expected output
	if ! diff -u "$temp_output_file" "$expected_file" 2>/dev/null >"$temp_diff"; then
		echo "ERROR: for '${args[*]}': actual and expected outputs differ:" >&2
		cat "$temp_diff" >&2
		ret=1
	fi

	rm -f "$temp_output_file" "$temp_diff"

	return $ret
}

# Checks the difference between the content of the file with path "$1"
# and the output of the CLI when called on the directory path "$2" with
# the arguments '-c sink.text.details' and the rest of the arguments to
# this function.
#
# Returns 0 if there's no difference, and 1 if there is, also printing
# said difference to the standard error.
bt_diff_details_ctf_single() {
	local expected_file="$1"
	local trace_dir="$2"
	shift 2
	local extra_details_args=("$@")

	# Compare using the CLI with `sink.text.details`
	bt_diff_cli "$expected_file" "$trace_dir" "-c" "sink.text.details" "${extra_details_args[@]}"
}

# Calls bt_diff_details_ctf_single(), except that "$1" is the path to a
# program which generates the CTF trace to compare to. The program "$1"
# receives the path to a temporary, empty directory where to write the
# CTF trace as its first argument.
bt_diff_details_ctf_gen_single() {
	local ctf_gen_prog_path="$1"
	local expected_file="$2"
	shift 2
	local extra_details_args=("$@")

	local temp_trace_dir
	local ret

	temp_trace_dir="$(mktemp -d)"

	# Run the CTF trace generator program to get a CTF trace
	if ! "$ctf_gen_prog_path" "$temp_trace_dir" 2>/dev/null; then
		echo "ERROR: \"$ctf_gen_prog_path\" \"$temp_trace_dir\" failed" >&2
		rm -rf "$temp_trace_dir"
		return 1
	fi

	# Compare using the CLI with `sink.text.details`
	bt_diff_details_ctf_single "$expected_file" "$temp_trace_dir" "${extra_details_args[@]}"
	ret=$?
	rm -rf "$temp_trace_dir"
	return $ret
}


### Functions ###

check_coverage() {
	coverage run "$@"
}

# Execute a shell command in the appropriate environment to have access to the
# bt2 Python bindings.
run_python_bt2() {
	local lib_search_var
	local lib_search_path

	local main_lib_path="${BT_TESTS_BUILDDIR}/../src/lib/.libs"

	# Set the library search path so the python interpreter can load libbabeltrace2
	if [ "$BT_OS_TYPE" = "mingw" ]; then
		lib_search_var="PATH"
		lib_search_path="${main_lib_path}:${PATH:-}"
	elif [ "$BT_OS_TYPE" = "darwin" ]; then
		lib_search_var="DYLD_LIBRARY_PATH"
		lib_search_path="${main_lib_path}:${DYLD_LIBRARY_PATH:-}"
	else
		lib_search_var="LD_LIBRARY_PATH"
		lib_search_path="${main_lib_path}:${LD_LIBRARY_PATH:-}"
	fi

	env \
		BABELTRACE_PYTHON_BT2_NO_TRACEBACK=1 \
		BABELTRACE_PLUGIN_PATH="${BT_TESTS_BABELTRACE_PLUGIN_PATH}" \
		LIBBABELTRACE2_PLUGIN_PROVIDER_DIR=${BT_TESTS_PROVIDER_DIR} \
		BT_CTF_TRACES_PATH="${BT_CTF_TRACES_PATH}" \
		BT_PLUGINS_PATH="${BT_PLUGINS_PATH}" \
		PYTHONPATH="${BT_TESTS_PYTHONPATH}:${BT_TESTS_SRCDIR}/utils/python" \
		"${lib_search_var}"="${lib_search_path}" \
		"$@"
}

# Set the environment and run python tests in the directory.
#
# $1 : The directory containing the python test scripts
# $2 : The pattern to match python test script names (optional)
run_python_bt2_test() {
	local test_dir="$1"
	local test_pattern="${2:-'*'}" # optional, if none default to "*"

	local ret
	local test_runner_args=()

	test_runner_args+=("$test_dir")
	if [ "x${test_pattern}" != "x" ]; then
		test_runner_args+=("${test_pattern}")
	fi

	if test "x${BT_TESTS_COVERAGE:-}" = "x1"; then
		python_exec="check_coverage"
	else
		python_exec="${BT_TESTS_PYTHON_BIN}"
	fi

	run_python_bt2 \
		"${python_exec}" \
		"${BT_TESTS_SRCDIR}/utils/python/testrunner.py" \
		--pattern "$test_pattern" \
		"$test_dir" \

	ret=$?

	if test "x${BT_TESTS_COVERAGE_REPORT:-}" = "x1"; then
		coverage report -m
	fi

	if test "x${BT_TESTS_COVERAGE_HTML:-}" = "x1"; then
		coverage html
	fi

	return $ret
}
