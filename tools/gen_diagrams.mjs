import { readdir, stat } from "node:fs/promises";
import { spawn } from "node:child_process";
import { basename, extname, join } from "node:path";
import { fileURLToPath } from "node:url";
import process from "node:process";

const diagramsDir = fileURLToPath(new URL("../docs/diagrams/", import.meta.url));

const mmdcArgs = (input, output) => [
  "-i",
  input,
  "-o",
  output,
  "-b",
  "white",
  "-s",
  "2",
  "--quiet",
];

const run = (file, args, shell) =>
  new Promise((resolve) => {
    const child = spawn(file, args, { shell, stdio: ["ignore", "pipe", "pipe"] });
    let stderr = "";
    child.stderr.setEncoding("utf8");
    child.stderr.on("data", (chunk) => {
      stderr += chunk;
    });
    child.on("error", (error) => resolve({ ok: false, error, stderr }));
    child.on("close", (code) => resolve({ ok: code === 0, code, stderr }));
  });

const render = async (input, output) => {
  const args = mmdcArgs(input, output);
  const direct = await run("mmdc", args, false);
  if (!direct.error || direct.error.code !== "ENOENT") return direct;
  const command = [
    "npx",
    "-y",
    "@mermaid-js/mermaid-cli",
    ...args.map((value) => `"${value}"`),
  ].join(" ");
  return run(command, [], true);
};

const main = async () => {
  const names = (await readdir(diagramsDir))
    .filter((name) => extname(name) === ".mmd")
    .sort();
  let failures = 0;
  for (const name of names) {
    const input = join(diagramsDir, name);
    const output = join(diagramsDir, `${basename(name, ".mmd")}.png`);
    const result = await render(input, output);
    if (!result.ok) {
      failures += 1;
      console.error(`failed: ${name}`);
      if (result.error) console.error(result.error.message);
      if (result.stderr.trim()) console.error(result.stderr.trim());
      continue;
    }
    const { size } = await stat(output);
    console.log(`ok: ${basename(output)} (${size} bytes)`);
  }
  if (failures > 0) process.exitCode = 1;
};

main().catch((error) => {
  console.error(error);
  process.exitCode = 1;
});
