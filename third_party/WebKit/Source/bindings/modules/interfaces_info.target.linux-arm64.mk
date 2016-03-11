# This file is generated by gyp; do not edit.

include $(CLEAR_VARS)

LOCAL_MODULE_CLASS := GYP
LOCAL_MODULE := third_party_WebKit_Source_bindings_modules_interfaces_info_gyp
LOCAL_MODULE_STEM := interfaces_info
LOCAL_MODULE_SUFFIX := .stamp
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_TARGET_ARCH := $(TARGET_$(GYP_VAR_PREFIX)ARCH)
gyp_intermediate_dir := $(call local-intermediates-dir,,$(GYP_VAR_PREFIX))
gyp_shared_intermediate_dir := $(call intermediates-dir-for,GYP,shared,,,$(GYP_VAR_PREFIX))

# Make sure our deps are built first.
GYP_TARGET_DEPENDENCIES := \
	$(call intermediates-dir-for,GYP,third_party_WebKit_Source_bindings_core_interfaces_info_individual_core_gyp,,,$(GYP_VAR_PREFIX))/interfaces_info_individual_core.stamp \
	$(call intermediates-dir-for,GYP,third_party_WebKit_Source_bindings_modules_interfaces_info_individual_modules_gyp,,,$(GYP_VAR_PREFIX))/interfaces_info_individual_modules.stamp

### Rules for action "compute_interfaces_info":
$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModules.pickle: gyp_local_path := $(LOCAL_PATH)
$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModules.pickle: gyp_var_prefix := $(GYP_VAR_PREFIX)
$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModules.pickle: gyp_intermediate_dir := $(abspath $(gyp_intermediate_dir))
$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModules.pickle: gyp_shared_intermediate_dir := $(abspath $(gyp_shared_intermediate_dir))
$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModules.pickle: export PATH := $(subst $(ANDROID_BUILD_PATHS),,$(PATH))
$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModules.pickle: $(LOCAL_PATH)/third_party/WebKit/Source/bindings/scripts/compute_interfaces_info_overall.py $(gyp_shared_intermediate_dir)/blink/bindings/core/InterfacesInfoCoreIndividual.pickle $(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModulesIndividual.pickle $(GYP_TARGET_DEPENDENCIES)
	@echo "Gyp action: Computing overall global information about IDL files for interfaces_info ($@)"
	$(hide)cd $(gyp_local_path)/third_party/WebKit/Source/bindings/modules; mkdir -p $(gyp_shared_intermediate_dir)/blink/bindings/modules; python ../scripts/compute_interfaces_info_overall.py --write-file-only-if-changed 0 -- "$(gyp_shared_intermediate_dir)/blink/bindings/core/InterfacesInfoCoreIndividual.pickle" "$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModulesIndividual.pickle" "$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModules.pickle"



GYP_GENERATED_OUTPUTS := \
	$(gyp_shared_intermediate_dir)/blink/bindings/modules/InterfacesInfoModules.pickle

# Make sure our deps and generated files are built first.
LOCAL_ADDITIONAL_DEPENDENCIES := $(GYP_TARGET_DEPENDENCIES) $(GYP_GENERATED_OUTPUTS)

### Rules for final target.
# Add target alias to "gyp_all_modules" target.
.PHONY: gyp_all_modules
gyp_all_modules: third_party_WebKit_Source_bindings_modules_interfaces_info_gyp

# Alias gyp target name.
.PHONY: interfaces_info
interfaces_info: third_party_WebKit_Source_bindings_modules_interfaces_info_gyp

LOCAL_MODULE_PATH := $(PRODUCT_OUT)/gyp_stamp
LOCAL_UNINSTALLABLE_MODULE := true
LOCAL_2ND_ARCH_VAR_PREFIX := $(GYP_VAR_PREFIX)

include $(BUILD_SYSTEM)/base_rules.mk

$(LOCAL_BUILT_MODULE): $(LOCAL_ADDITIONAL_DEPENDENCIES)
	$(hide) echo "Gyp timestamp: $@"
	$(hide) mkdir -p $(dir $@)
	$(hide) touch $@

LOCAL_2ND_ARCH_VAR_PREFIX :=
