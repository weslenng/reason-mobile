open Helper;

// TODO: use Path.t
let read_file = path => {
  // TODO: do I need to close it?
  let.await file = Lwt_io.open_file(~mode=Input, path);
  Lwt_io.read(file);
};
let write_file = (~file, data) => {
  // TODO: do I need to close it?
  let.await file = Lwt_io.open_file(~mode=Output, file);
  Lwt_io.write(file, data);
};

// TODO: sh -c will fail when cmd length > MAX_ARG_STRLEN
let exec = cmd => cmd |> Lwt_process.shell |> Lwt_process.pread;

let mkdirp = folder => exec("mkdir -p " ++ folder) |> Lwt.map(_ => ());

let sha1 = str =>
  Cstruct.of_string(str)
  |> Mirage_crypto.Hash.SHA1.digest
  |> Cstruct.to_string
  |> Hex.of_string
  |> Hex.show;

// TODO: use Path.t
let folder_sha1 = path => {
  // TODO: exclude .mocks and _esy to keep performance
  let.await find = exec("find " ++ path);
  let.await hashes =
    find
    |> String.split_on_char('\n')
    |> List.map(String.trim)
    |> List.filter((!=)(""))
    |> List.map(file => {
         let.await stats = Lwt_unix.stat(file);
         switch (stats) {
         // TODO: what should happen when is symlink?
         | {Unix.st_kind: Unix.S_REG, _} =>
           let.await file_data = read_file(file);
           Lwt.return(Some(sha1(file_data)));
         | _ => Lwt.return(None)
         };
       })
    |> Lwt.all;
  hashes
  |> List.filter_map(Fun.id)
  |> String.concat(" ")
  |> sha1
  |> Lwt.return;
};

let replace_all = (~pattern, ~by, str) =>
  Re.Str.global_replace(Re.Str.regexp_string(pattern), by, str);
let starts_with = (~pattern, name) => {
  let length = String.length(pattern);
  String.length(name) >= length && String.sub(name, 0, length) == pattern;
};

let rev_string = s =>
  s |> String.to_seq |> List.of_seq |> List.rev |> List.to_seq |> String.of_seq;
let ends_with = (~pattern, name) =>
  starts_with(~pattern=rev_string(pattern), rev_string(name));

let escape_name = Re.replace(~f=_ => "_", Re.Pcre.regexp("@|\\/|:|-"));
let target_name = (target, name) =>
  switch (target) {
  | `Host => name
  | `Target(target_name) => "@_" ++ target_name ++ "/" ++ escape_name(name)
  };
