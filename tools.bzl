"""Rule for downloading and verifying test data"""

def _download_data_impl(ctx):
    # Fetch the URL and SHA-256 from attributes
    url = ctx.attr.url
    expected_sha256 = ctx.attr.sha256
    output_file = ctx.actions.declare_file(ctx.label.name + ".json")  # Downloaded file
    hash_output = ctx.actions.declare_file(ctx.label.name + ".sha256")  # File to store computed hash

    # Determine if we're on Windows based on the host platform
    is_windows = ctx.var.get("HOST_OS", "linux").lower() == "windows"

    if is_windows:
        # Windows-specific commands
        download_command = 'powershell -Command "Invoke-WebRequest -Uri \\"%s\\" -OutFile \\"%s\\""' % (url, output_file.path)

        # Use certutil for SHA256 on Windows
        hash_command = 'certutil -hashfile "%s" SHA256 | findstr /v "hash" > "%s"' % (output_file.path, hash_output.path)
        verify_command = """
            set /p computed=<"%s"
            set "computed=!computed: =!"
            if /i "!computed!" NEQ "%s" (
                echo SHA-256 mismatch!
                echo Expected: %s
                echo Got:      !computed!
                exit /b 1
            )
        """ % (hash_output.path, expected_sha256, expected_sha256)
        full_command = " && ".join([download_command, hash_command, verify_command])
    else:
        # Unix-specific commands
        download_command = "curl -s %s -o %s" % (url, output_file.path)
        hash_command = "openssl dgst -sha256 %s | cut -d' ' -f2 > %s" % (output_file.path, hash_output.path)
        verify_command = """
            computed=$(cat %s)
            if [ "$computed" != "%s" ]; then
                echo "SHA-256 mismatch!"
                echo "Expected: %s"
                echo "Got:      $computed"
                exit 1
            fi
        """ % (hash_output.path, expected_sha256, expected_sha256)
        full_command = " && ".join([download_command, hash_command, verify_command])

    # Execute the download and verification
    ctx.actions.run_shell(
        inputs = [],
        outputs = [output_file, hash_output],
        command = full_command,
        mnemonic = "DownloadAndVerify",
        progress_message = "Downloading and verifying test data from %s" % url,
        arguments = ["/C" if is_windows else "-c", full_command],  # Specify the shell to use
    )

    # Return the downloaded file as a provider
    return [DefaultInfo(files = depset([output_file]))]

download_data = rule(
    implementation = _download_data_impl,
    attrs = {
        "url": attr.string(mandatory = True, doc = "URL of the data file to download"),
        "sha256": attr.string(mandatory = True, doc = "Expected SHA-256 checksum of the downloaded file"),
    },
)
