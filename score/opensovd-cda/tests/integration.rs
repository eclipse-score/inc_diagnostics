// *******************************************************************************
// Copyright (c) 2026 Contributors to the Eclipse Foundation
//
// See the NOTICE file(s) distributed with this work for additional
// information regarding copyright ownership.
//
// This program and the accompanying materials are made available under the
// terms of the Apache License Version 2.0 which is available at
// <https://www.apache.org/licenses/LICENSE-2.0>
//
// SPDX-License-Identifier: Apache-2.0
// *******************************************************************************

use opensovd_client::Client;
use std::{
    env,
    fs,
    path::{Path, PathBuf},
    process::{Child, Command, Stdio},
    time::Duration,
};

struct ChildGuard(Child);

impl Drop for ChildGuard {
    fn drop(&mut self) {
        let _ = self.0.kill();
        let _ = self.0.wait();
    }
}

fn runfile(path: &str) -> PathBuf {
    let root = PathBuf::from(env::var_os("TEST_SRCDIR").expect("TEST_SRCDIR"));
    let workspace = env::var("TEST_WORKSPACE").expect("TEST_WORKSPACE");
    let candidates = [
        root.join(&workspace).join(path),
        root.join("_main").join(path),
        root.join(path),
    ];
    candidates
        .into_iter()
        .find(|candidate| candidate.exists())
        .unwrap_or_else(|| panic!("runfile not found: {path}"))
}

fn write_config(path: &Path, database: &Path, storage: &Path, port: u16) {
    let config = format!(
        "[database]\npath = {:?}\nexit_no_database_loaded = false\n\n\
         [doip]\ntester_address = \"127.0.0.1\"\n\n\
         [runtime_update_config]\nstorage_dir = {:?}\n\n\
         [server]\naddress = \"127.0.0.1\"\nport = {port}\n",
        database.display().to_string(),
        storage.display().to_string(),
    );
    fs::write(path, config).expect("write CDA config");
}

#[tokio::test(flavor = "current_thread")]
async fn cda_serves_version_and_components() {
    let binary = runfile("score/opensovd-cda/opensovd-cda");
    let mdd = runfile("score/opensovd-cda/mdd/functional_groups.mdd");
    let temp = env::temp_dir().join(format!("inc-diagnostics-cda-{}", std::process::id()));
    fs::create_dir_all(&temp).expect("create test directory");
    let config = temp.join("cda.toml");
    let storage = temp.join("storage");
    fs::create_dir_all(&storage).expect("create storage directory");
    write_config(
        &config,
        mdd.parent().expect("MDD parent"),
        &storage,
        24000 + (std::process::id() % 10000) as u16,
    );

    let port = 24000 + (std::process::id() % 10000) as u16;
    let mut child = ChildGuard(
        Command::new(binary)
            .arg("--config")
            .arg(&config)
            .stdout(Stdio::null())
            .stderr(Stdio::null())
            .spawn()
            .expect("spawn CDA"),
    );
    let url = format!("http://127.0.0.1:{port}/vehicle/v15");
    let mut last_error = None;
    for _ in 0..120 {
        match Client::connect(&url) {
            Ok(client) => {
                match client.list_components().send().await {
                    Ok(components) => {
                        assert!(components.data.items.is_empty());
                        return;
                    }
                    Err(error) => last_error = Some(error.to_string()),
                }
            }
            Err(error) => last_error = Some(error.to_string()),
        }
        tokio::time::sleep(Duration::from_millis(250)).await;
    }
    let _ = child.0.try_wait();
    panic!("CDA did not become reachable: {last_error:?}");
}
