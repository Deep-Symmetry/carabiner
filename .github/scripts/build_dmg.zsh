# Initialize our nested projects.
git submodule update --init --recursive

# Build the native executable.
mkdir build
cd build
cmake ..
cmake --build .

# See if the secrets needed to code-sign the disk image are present.
if  [ "$IDENTITY_PASSPHRASE" != "" ]; then

    # We have secrets! Set up a keychain to hold the signing certificate. We can use the same
    # secret passphrase that we will use to import it as the keychain password, for simplicity.
    security create-keychain -p "$IDENTITY_PASSPHRASE" build.keychain
    security default-keychain -s build.keychain
    security unlock-keychain -p "$IDENTITY_PASSPHRASE" build.keychain

    # Put the base-64 encoded signing certificicate into a text file, decode it to binary form.
    echo "$IDENTITY_P12_B64" > DS_ID_App.p12.txt
    openssl base64 -d -in DS_ID_App.p12.txt -out DS_ID_App.p12

    # Install the decoded signing certificate into our unlocked build keychain.
    security import DS_ID_App.p12 -A -P "$IDENTITY_PASSPHRASE"

    # Set the keychain to allow use of the certificate without user interaction (we are headless!)
    security set-key-partition-list -S apple-tool:,apple: -s -k "$IDENTITY_PASSPHRASE" build.keychain

    # Code sign the binary.
    echo "Code signing the executable."
    codesign --verbose --timestamp --options runtime --sign $mac_signing_name bin/Carabiner

    # Wrap the signed executable in a disk image
    hdiutil create $mac_dmg_name -fs HFS+ -volname 'Carabiner' -srcfolder bin

    # Also code sign the disk image?!
    echo "Code signing the disk image."
    codesign --verbose --deep --timestamp --options runtime --sign $mac_signing_name \
             --prefix "org.deepsymmetry.Carabiner." $mac_dmg_name

    # Submit the disk image to Apple for notarization.
    echo "Sumbitting the disk image to Apple for notarization..."
    xcrun altool --notarize-app --primary-bundle-id "org.deepsymmetry.carabiner" \
          --username "$mac_notarization_user" --password "$NOTARIZATION_PW" \
          --file "$mac_dmg_name" --output-format xml > upload_result.plist
    request_id=`/usr/libexec/PlistBuddy -c "Print :notarization-upload:RequestUUID" upload_result.plist`

    # Wait until the request is done processing.
    while true; do
        sleep 60
        xcrun altool --notarization-info $request_id \
              --username "$mac_notarization_user" --password "$NOTARIZATION_PW" \
              --output-format xml > status.plist
        if [ "`/usr/libexec/PlistBuddy -c "Print :notarization-info:Status" status.plist`" != "in progress" ]; then
            break;
        fi
        echo "...still waiting for notarization to finish..."
    done

    # See if notarization succeeded, and if so, staple the ticket to the disk image.
    if [ `/usr/libexec/PlistBuddy -c "Print :notarization-info:Status" status.plist` = "success" ]; then
        echo "Notarization succeeded, stapling receipt to disk image."
        xcrun stapler staple "$mac_dmg_name"
    else
        false;
    fi
else
    # No secrets, just wrap the unsigned executable in a disk image.
    hdiutil create $mac_dmg_name -volname 'Carabiner' -srcfolder bin
fi

# Move the disk image to where the upload task expects to find it.
mv $mac_dmg_name ..
