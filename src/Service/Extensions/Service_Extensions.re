open Oni_Core;

module Log = (val Log.withNamespace("Service.Extensions"));

module Constants = {
  let baseUrl = "https://open-vsx.org/api";
};

module Url = {
  let extensionInfo = (publisher, id) => {
    Printf.sprintf("%s/%s/%s", Constants.baseUrl, publisher, id);
  };
};

module Catalog = {
  module VersionInfo = {
    type t = {
      version: string,
      url: string,
    };

    let decode =
      Json.Decode.(
        key_value_pairs(string)
        |> map(List.map(((version, url)) => {version, url}))
      );

    let toString = ({version, url}) =>
      Printf.sprintf(" - Version %s: %s", version, url);
  };

  module Entry = {
    type t = {
      downloadUrl: string,
      repositoryUrl: string,
      homepageUrl: string,
      manifestUrl: string,
      iconUrl: string,
      readmeUrl: string,
      licenseName: string,
      //      licenseUrl: string,
      //      name: string,
      //      namespace: string,
      //      downloadCount: int,
      displayName: string,
      description: string,
      //      categories: list(string),
      versions: list(VersionInfo.t),
    };

    let toString =
        ({downloadUrl, displayName, description, homepageUrl, versions, _}) => {
      let versions =
        versions |> List.map(VersionInfo.toString) |> String.concat("\n");
      Printf.sprintf(
        {|Extension %s:
- Description: %s
- Homepage: %s
- Download Url: %s
- Versions:
%s
      |},
        displayName,
        description,
        homepageUrl,
        downloadUrl,
        versions,
      );
    };

    module Decode = {
      open Json.Decode;

      let files = (name, decoder) => field("files", field(name, decoder));
      let downloadUrl = files("download", string);
      let manifestUrl = files("manifest", string);
      let iconUrl = files("icon", string);
      let readmeUrl = files("readme", string);
      let homepageUrl = field("publishedBy", field("homepage", string));

      let decode =
        obj(({field, whatever, _}) =>
          {
            downloadUrl: whatever(downloadUrl),
            manifestUrl: whatever(manifestUrl),
            iconUrl: whatever(iconUrl),
            readmeUrl: whatever(readmeUrl),
            repositoryUrl: field.required("repository", string),
            homepageUrl: whatever(homepageUrl),
            licenseName: field.required("license", string),
            displayName: field.required("displayName", string),
            description: field.required("description", string),
            versions:
              field.withDefault("allVersions", [], VersionInfo.decode),
          }
        );
    };
  };

  let toPublisherName = str => {
    let items = String.split_on_char('.', str);
    switch (items) {
    | [publisher, id] => Some((publisher, id))
    | _ => None
    };
  };

  let query = (~setup, id) => {
    switch (toPublisherName(id)) {
    | None => Lwt.fail_with("Invalid id: " ++ id)
    | Some((publisher, name)) =>
      let url = Url.extensionInfo(publisher, name);

      Service_Net.Request.json(~setup, ~decoder=Entry.Decode.decode, url);
    };
  };
};

module Management = {
let install = (~setup,~extensionsFolder, path) => {
  let name = Rench.Path.filename(path);

  let absolutePath =
    if (Rench.Path.isAbsolute(path)) {
      path;
    } else {
      Rench.Path.join(Rench.Environment.getWorkingDirectory(), path);
    };

  Log.debugf(m => m("Installing extension %s to %s", name, extensionsFolder));

  let promise: Lwt.t(string) =
    NodeTask.run(
      ~name="Install",
      ~setup,
      ~args=[absolutePath, extensionsFolder],
      "install-extension.js",
    );

  Lwt.on_success(promise, _ => {
    Log.debugf(m => m("Successfully installed extension: %s", name))
  });

  Lwt.on_failure(promise, _ => {
    Log.errorf(m => m("Unable to install extension: %s", name))
  });
  promise;
};

let uninstall = 
  (~extensionsFolder, id) => {
    // TODO: Implement this
    ignore(extensionsFolder);
    ignore(id);
    Lwt.return();
  };
}