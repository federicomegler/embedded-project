webtalk_init -webtalk_dir V:\\software\\m3_for_arty_a7\\sdk_workspace\\webtalk
webtalk_register_client -client project
webtalk_add_data -client project -key date_generated -value "2021-02-19 16:46:30" -context "software_version_and_target_device"
webtalk_add_data -client project -key product_version -value "SDK v2019.1" -context "software_version_and_target_device"
webtalk_add_data -client project -key build_version -value "2019.1" -context "software_version_and_target_device"
webtalk_add_data -client project -key os_platform -value "amd64" -context "software_version_and_target_device"
webtalk_add_data -client project -key registration_id -value "" -context "software_version_and_target_device"
webtalk_add_data -client project -key tool_flow -value "SDK" -context "software_version_and_target_device"
webtalk_add_data -client project -key beta -value "false" -context "software_version_and_target_device"
webtalk_add_data -client project -key route_design -value "NA" -context "software_version_and_target_device"
webtalk_add_data -client project -key target_family -value "NA" -context "software_version_and_target_device"
webtalk_add_data -client project -key target_device -value "NA" -context "software_version_and_target_device"
webtalk_add_data -client project -key target_package -value "NA" -context "software_version_and_target_device"
webtalk_add_data -client project -key target_speed -value "NA" -context "software_version_and_target_device"
webtalk_add_data -client project -key random_id -value "hdga5odmekskojnpikp7joa22n" -context "software_version_and_target_device"
webtalk_add_data -client project -key project_id -value "2019.1_8" -context "software_version_and_target_device"
webtalk_add_data -client project -key project_iteration -value "8" -context "software_version_and_target_device"
webtalk_add_data -client project -key os_name -value "Microsoft Windows 8 or later , 64-bit" -context "user_environment"
webtalk_add_data -client project -key os_release -value "major release  (build 9200)" -context "user_environment"
webtalk_add_data -client project -key cpu_name -value "Intel(R) Core(TM) i9-9880H CPU @ 2.30GHz" -context "user_environment"
webtalk_add_data -client project -key cpu_speed -value "2304 MHz" -context "user_environment"
webtalk_add_data -client project -key total_processors -value "1" -context "user_environment"
webtalk_add_data -client project -key system_ram -value "6.419 GB" -context "user_environment"
webtalk_register_client -client sdk
webtalk_add_data -client sdk -key uid -value "1613747189349" -context "sdk\\\\bsp/1613747189349"
webtalk_add_data -client sdk -key hwid -value "1613745945545" -context "sdk\\\\bsp/1613747189349"
webtalk_add_data -client sdk -key os -value "standalone" -context "sdk\\\\bsp/1613747189349"
webtalk_add_data -client sdk -key apptemplate -value "null" -context "sdk\\\\bsp/1613747189349"
webtalk_add_data -client sdk -key RecordType -value "BSPCreation" -context "sdk\\\\bsp/1613747189349"
webtalk_add_data -client sdk -key uid -value "NA" -context "sdk\\\\bsp/1613749590838"
webtalk_add_data -client sdk -key RecordType -value "ToolUsage" -context "sdk\\\\bsp/1613749590838"
webtalk_add_data -client sdk -key BootgenCount -value "0" -context "sdk\\\\bsp/1613749590838"
webtalk_add_data -client sdk -key DebugCount -value "0" -context "sdk\\\\bsp/1613749590838"
webtalk_add_data -client sdk -key PerfCount -value "0" -context "sdk\\\\bsp/1613749590838"
webtalk_add_data -client sdk -key FlashCount -value "0" -context "sdk\\\\bsp/1613749590838"
webtalk_add_data -client sdk -key CrossTriggCount -value "0" -context "sdk\\\\bsp/1613749590838"
webtalk_add_data -client sdk -key QemuDebugCount -value "0" -context "sdk\\\\bsp/1613749590838"
webtalk_transmit -clientid 3409529040 -regid "" -xml V:\\software\\m3_for_arty_a7\\sdk_workspace\\webtalk\\usage_statistics_ext_sdk.xml -html V:\\software\\m3_for_arty_a7\\sdk_workspace\\webtalk\\usage_statistics_ext_sdk.html -wdm V:\\software\\m3_for_arty_a7\\sdk_workspace\\webtalk\\sdk_webtalk.wdm -intro "<H3>SDK Usage Report</H3><BR>"
webtalk_terminate
