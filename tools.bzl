"""Rule for downloading and verifying test data"""

def _download_data_impl(ctx):
    # Fetch the URL and SHA-256 from attributes
    url = ctx.attr.url
    expected_sha256 = ctx.attr.sha256
    output_file = ctx.actions.declare_file(ctx.label.name + ".json")  # Downloaded file
    hash_output = ctx.actions.declare_file(ctx.label.name + ".sha256")  # File to store computed hash

    # Download the file and compute its SHA-256 hash
    download_command = "curl -s %s > %s" % (url, output_file.path)
    hash_command = "openssl dgst -sha256 %s | cut -d' ' -f2 > %s" % (output_file.path, hash_output.path)
    verify_command = """
        computed=$(cat {hash_output})
        if [ "$computed" != "{expected_sha256}" ]; then
            echo "SHA-256 mismatch!"
            echo "Expected: {expected_sha256}"
            echo "Got:      $computed"
            exit 1
        fi
    """.format(
        hash_output = hash_output.path,
        expected_sha256 = expected_sha256,
    )

    # Combine all commands into a single shell script
    full_command = " && ".join([
        download_command,
        hash_command,
        verify_command,
    ])

    # Execute the download and verification
    ctx.actions.run_shell(
        inputs = [],
        outputs = [output_file, hash_output],
        command = full_command,
        mnemonic = "DownloadAndVerify",
        progress_message = "Downloading and verifying test data from %s" % url,
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
