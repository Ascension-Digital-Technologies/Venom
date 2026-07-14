# Annotation reference

| Annotation | Scope | Behavior |
|---|---|---|
| none | file/function | protected by default where compatible |
| `// @venom: browser` | file | keep the file browser-executed |
| `// @venom: browser` | function | keep that function browser-executed |
| `// @venom: protected` | file/function | explicitly require protected planning |
| `// @venom: protected isolated` | function | expose an isolated protected export |
