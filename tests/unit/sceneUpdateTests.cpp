#include "catch.hpp"

#include "yaml-cpp/yaml.h"
#include "scene/sceneLoader.h"
#include "style/style.h"
#include "scene/scene.h"
#include "log.h"
#include "tangram.h"

using YAML::Node;
using namespace Tangram;

const static std::string sceneString = R"END(
global:
    a: global_a_value
    b: global_b_value
map:
    a: map_a_value
    b: global.b
seq:
    - seq_0_value
    - global.a
nest:
    map:
        a: nest_map_a_value
        b: nest_map_b_value
    seq:
        - nest_seq_0_value
        - nest_seq_1_value
)END";

bool loadConfig(const std::string& _sceneString, Node& root) {

    try { root = YAML::Load(_sceneString); }
    catch (YAML::ParserException e) {
        LOGE("Parsing scene config '%s'", e.what());
        return false;
    }
    return true;
}

TEST_CASE("Apply scene update to a top-level node") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    CHECK(root["map"].Scalar() == "new_value");
}

TEST_CASE("Apply scene update to a map entry") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map.a", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    CHECK(root["map"]["a"].Scalar() == "new_value");
    // Check that nearby values are unchanged.
    CHECK(root["map"]["b"].Scalar() == "global.b");
}

TEST_CASE("Apply scene update to a nested map entry") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"nest.map.a", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    CHECK(root["nest"]["map"]["a"].Scalar() == "new_value");
    // Check that nearby values are unchanged.
    CHECK(root["nest"]["map"]["b"].Scalar() == "nest_map_b_value");
}

TEST_CASE("Apply scene update to a sequence node") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"seq", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    CHECK(root["seq"].Scalar() == "new_value");
}

TEST_CASE("Apply scene update to a nested sequence node") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"nest.seq", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    CHECK(root["nest"]["seq"].Scalar() == "new_value");
    // Check that nearby values are unchanged.
    CHECK(root["nest"]["map"]["a"].Scalar() == "nest_map_a_value");
}

TEST_CASE("Apply scene update to a new map entry") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map.c", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    CHECK(root["map"]["c"].Scalar() == "new_value");
    // Check that nearby values are unchanged.
    CHECK(root["map"]["b"].Scalar() == "global.b");
}

TEST_CASE("Do not apply scene update to a non-existent node") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"none.a", "new_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    REQUIRE(!root["none"]);
}

TEST_CASE("Apply scene update that removes a node") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"nest.map", "null"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    CHECK(!root["nest"]["map"]["a"]);
    CHECK(root["nest"]["map"].IsNull());
    CHECK(root["nest"]["seq"]);
}

TEST_CASE("Apply multiple scene updates in order of request") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    // Add an update.
    std::vector<SceneUpdate> updates = {{"map.a", "first_value"}, {"map.a", "second_value"}};
    // Apply scene updates, reload scene.
    SceneLoader::applyUpdates(scene, updates);
    const Node& root = scene.config();
    CHECK(root["map"]["a"].Scalar() == "second_value");
    // Check that nearby values are unchanged.
    CHECK(root["map"]["b"].Scalar() == "global.b");
}

TEST_CASE("Apply and propogate repeated global value updates") {
    // Setup.
    Scene scene;
    REQUIRE(loadConfig(sceneString, scene.config()));
    Node& root = scene.config();
    // Apply initial globals.
    SceneLoader::applyGlobals(root, scene);
    CHECK(root["seq"][1].Scalar() == "global_a_value");
    CHECK(root["map"]["b"].Scalar() == "global_b_value");
    // Add an update.
    std::vector<SceneUpdate> updates = {{"global.b", "new_global_b_value"}};
    // Apply the update.
    SceneLoader::applyUpdates(scene, updates);
    CHECK(root["global"]["b"].Scalar() == "new_global_b_value");
    // Apply updated globals.
    SceneLoader::applyGlobals(root, scene);
    CHECK(root["seq"][1].Scalar() == "global_a_value");
    CHECK(root["map"]["b"].Scalar() == "new_global_b_value");
    // Add an update.
    updates = {{"global.b", "newer_global_b_value"}};
    // Apply the update.
    SceneLoader::applyUpdates(scene, updates);
    CHECK(root["global"]["b"].Scalar() == "newer_global_b_value");
    // Apply updated globals.
    SceneLoader::applyGlobals(root, scene);
    CHECK(root["seq"][1].Scalar() == "global_a_value");
    CHECK(root["map"]["b"].Scalar() == "newer_global_b_value");
}
