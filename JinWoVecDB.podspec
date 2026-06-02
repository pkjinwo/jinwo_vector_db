Pod::Spec.new do |s|
  s.name         = "JinWoVecDB"
  s.version      = "0.1.30"
  s.summary      = "JinWo VecDB - Fast embedding vector database for iOS"
  s.description  = <<-DESC
    JinWo VecDB is a high-performance vector database supporting efficient
    embedding storage and fast KNN search on mobile devices.
  DESC
  s.homepage     = "https://github.com/pkjinwo/jinwo_vector_db"
  s.license      = { :type => "MIT", :file => "LICENSE" }
  s.author       = { "pkjinwo" => "pkjinwo@users.noreply.github.com" }
  s.source       = { :git => "https://github.com/pkjinwo/jinwo_vector_db.git", :tag => "v#{s.version}" }

  s.ios.deployment_target = "13.0"
  s.static_framework = true

  # Swift + C sources
  s.source_files =
    "bindings/ios/Sources/**/*.swift",
    "src/*.c",
    "include/*.h"

  s.pod_target_xcconfig = {
    "HEADER_SEARCH_PATHS" => "$(PODS_TARGET_SRCROOT)/include",
    "SWIFT_INCLUDE_PATHS" => "$(PODS_TARGET_SRCROOT)/include",
    "OTHER_CFLAGS" => "-std=c11 -O2"
  }

  s.swift_version = "5.0"
end
