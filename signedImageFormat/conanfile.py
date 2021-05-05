from conans import ConanFile, tools

class SignedImageFormat(ConanFile):
    name = "signedImageFormat"
    version = "1.0"

    scm = {"type": "git",
           "url": "git@https://gitlab.esperanto.ai/software/signedImageFormat.git",
           "revision": "auto"}

    no_copy_source = True

    def package(self):
        self.copy("*.h")

    def package_id(self):
        self.info.header_only()
