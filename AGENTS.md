# Agent Instructions

- The Unreal Engine project is `HideSeek/HideSeek.uproject`.
- Treat HideSeek as an Unreal Engine 5.7 project.
- Version check: `HideSeek/HideSeek.uproject` has `"EngineAssociation": "5.7"`, and both `HideSeek.Target.cs` and `HideSeekEditor.Target.cs` use `EngineIncludeOrderVersion.Unreal5_7`.
- Prefer UE 5.7-compatible APIs and build settings when editing C++ or project configuration.
- After a build finishes and Unreal Editor can be launched, open the editor for `HideSeek/HideSeek.uproject` unless the user says not to.
- For Windows packaging with `RunUAT BuildCookRun`, request escalated permissions up front because AutomationTool writes AppData logs/caches and fails under the sandbox.
- Record completed user instructions in `Docs/requests.md` at the end of the task, using `## YYYY-MM-DD HH:mm:ss (소요시간: HH:MM:SS)` as the entry heading. The timestamp is when the user request was received or work began, and elapsed time is measured from that start time until the task is completed or the final response is being prepared. Do not include a timezone suffix, do not use `(elapsed: ...)`, and do not write `00:00:00` as a placeholder.
- Record user questions and their answers separately in `Docs/questions.md` at the end of the response, using the same Korean timestamp and elapsed-time heading format. Do not include a timezone suffix in question log timestamps. Do not duplicate questions in `Docs/requests.md`.
