---
name: docs-readme
description: "Guide an agent to create or update docs/README.md with source-verified C4 diagrams, data models, and API documentation."
disable-model-invocation: true
---
# Repository Documentation: Architecture, Data Models, and APIs

## Description

As part of this repository documentation task, you will generate or update `docs/README.md` using evidence verified in the target repository. The page must help developers understand the system architecture, its data models, and its API contracts.

**IMPORTANT: This is a source-grounded documentation task.** It involves:

1. Auditing existing documentation and preserving useful, accurate material.
2. Tracing the repository's actual architecture and describing it with C4 context, container, and component diagrams.
3. Identifying data models, relationships, configuration, and persistence only where evidenced.
4. Documenting public APIs and distinguishing inbound contracts from outbound integrations.
5. Validating factual claims, diagrams, links, and the final change scope.

**This task does NOT involve**:

1. Changing application code, build behavior, API definitions, or runtime configuration.
2. Creating separate API reference pages or broad documentation sets.
3. Guessing missing design details, undocumented guarantees, or intended future behavior.

## ⚠️ CRITICAL: Documentation Scope and Source Accuracy

**Your only persistent output is `docs/README.md` in the unambiguously identified target repository.** Read and analyze any relevant repository files, but do not edit them.

This task is for **developer-facing repository documentation**. Your role is to:

1. **Inspect** relevant source, public interfaces, configuration/build metadata, and existing documentation.
2. **Verify** every architecture, model, and API claim against authoritative repository evidence.
3. **Document** verified findings in the existing README or create it if absent.
4. **Validate** diagrams, relative links, content preservation, and the final diff.

**You must NEVER**:

1. Modify source code, tests, build files, schemas, CI files, or documentation other than `docs/README.md`.
2. Invent or imply APIs, fields, relationships, deployment units, persistence, defaults, guarantees, or behavior.
3. Treat mocks, stubs, or illustrative examples as authoritative when the real interface definition is available.
4. Overwrite existing documentation without first assessing it and retaining accurate useful content.
5. Add secrets, real credentials, personal data, or production-like values that are not safe and necessary.

If the target repository is ambiguous, ask the user before editing. State uncertainty explicitly when repository evidence is incomplete; do not fill evidence gaps with assumptions.

## Purpose & Audience

The README is a practical entry point for developers and maintainers who need to understand, change, integrate with, or troubleshoot the component. It should answer:

- What is this component responsible for, and what lies outside its boundary?
- Which people, systems, processes, and integrations interact with it?
- What are its deployable/runtime units and key internal responsibilities?
- What important data structures and relationships does it use, and where are they stored or configured?
- What interfaces does it expose, and what external APIs or platform services does it call?
- Where can a reader find authoritative, detailed documentation already present in the repository?

Use precise technical language. Define an acronym on first use when that improves readability. Explain enough context for a new maintainer without repeating every field or operation that belongs in detailed source references.

## Evidence Organization

Use a temporary, unique research tracker at `tmp/docs-readme-research-<task-id>.md` while working. Do not overwrite an existing tracker; choose a new unique task ID if the path exists. Record source paths and concise evidence, not copied source dumps. The tracker is scratch work, not a deliverable; remove only the exact tracker created by this task after validation.

Classify evidence before writing:

1. **Architecture**: runtime entry points, plugin/service registration, process boundaries, deployment/install metadata, and integration call sites.
2. **Data**: structs/classes/enums, serialization definitions, configuration, state transitions, persistence code, and ownership/relationships.
3. **Interfaces**: public API definitions, protocol/service identity, operations, events, payload types, and call sites showing direction.
4. **Documentation**: current `docs/README.md`, related guides, repository README, and documented conventions.

Prefer evidence in this order, as applicable: authoritative public API/schema and real headers; implementation and registration/configuration code; build/install metadata; current repository documentation; mocks or tests as corroboration only. For `ctrlm-main`, treat `ci/mocks/` and generated CI stubs as build scaffolding, not authoritative API definitions; prefer real installed headers under `install/usr/include/` and upstream interface definitions when available. Resolve conflicts in favor of the most direct authoritative evidence and record uncertainty rather than silently choosing an interpretation. Cite repository-relative source paths (and symbols or line numbers when stable and useful) near the claims they support.

## Required README Structure

Create a concise, navigable page with these sections, adjusting headings only when existing repository conventions make a clearer structure:

1. **Overview and responsibility boundary**: purpose, major responsibility, and explicit non-responsibilities when evidenced.
2. **Architecture**: three diagrams or clearly separated views:
   - **System context**: the software system, users/actors, and external systems.
   - **Containers**: deployable/runtime units and their communication paths. A container is an independently runnable or deployable unit, not merely a source folder.
   - **Components**: major responsibilities inside the relevant container, with dependencies.
   Follow each diagram with a short explanation. Label optional, build-gated, product-dependent, or unverified integrations accurately.
3. **Data models**: key entities/types, important relationships, configuration/state shape, and persistence/lifecycle only when implemented and verified. Link to authoritative definitions.
4. **APIs and integrations**: exposed/inbound contracts in a distinct subsection from outbound calls and platform dependencies. Identify protocol/transport, service identity or callsign, version/revision, representative operations/events, payload type sources, and compatibility caveats where evidenced.
5. **Related documentation**: links to existing build, configuration, API, deployment, and architecture resources only when those resources exist and the links resolve.

Do not force a section to claim details the repository does not establish. If a requested area is absent or unclear, say so briefly and specifically rather than omitting it without explanation.

## Documentation Guidelines

### 1. Ground claims in evidence

- **Good**: “The plugin registers the `ControlManager` service in `src/plugin.cpp`.”
- **Bad**: “The plugin is always launched by the system service manager.” (unless the repository proves this)
- **Why**: The good statement is specific and traceable; the bad one turns an assumption into a runtime guarantee.

### 2. Model boundaries precisely

- **Good**: Show a library as a component inside the process that links it.
- **Bad**: Draw every source directory as a separate C4 container.
- **Why**: C4 views explain runtime architecture, not folder organization.

### 3. Describe models without inventing schema

- **Good**: “`DeviceInfo` contains the fields declared in the public header; state is maintained in memory by the manager.”
- **Bad**: “The device database has a normalized `DeviceInfo` table.” (without schema and persistence evidence)
- **Why**: Source types and persistence behavior do not automatically imply a database schema.

### 4. Keep API direction explicit

Use Mermaid C4 diagram types (`C4Context`, `C4Container`, `C4Component`) when supported by the repository's Markdown renderer. Otherwise use a diagram format the repository already supports or a clear, structured text diagram; do not add tooling or dependencies just for rendering. Diagrams must have clear titles, meaningful relationships, and consistent system boundaries.

- **Good**: “Inbound: clients invoke the plugin's JSON-RPC methods. Outbound: the plugin calls DeviceSettings to query audio state.”
- **Bad**: Put both directions in one “API” list without identifying which interface the component serves.
- **Why**: Readers need to know which contracts they can call and which dependencies the implementation calls.

### 5. Preserve and improve existing material

- **Good**: Keep an accurate setup section, correct outdated links, and add the missing architecture and API sections.
- **Bad**: Replace the whole README with a generic template before inspecting it.
- **Why**: The task is to improve the project's documentation, not discard useful local knowledge.

### 6. Make links and examples dependable

- **Good**: Link to `../src/include/service.h` from `docs/README.md` and verify the resolved path exists.
- **Bad**: Add a plausible `api-reference.md` link without checking for that file.
- **Why**: Broken links and fabricated examples erode trust and waste readers' time.

### 7. Be concise and explicit about unknowns

- **Good**: “No persistent storage path was found in the reviewed implementation.”
- **Bad**: “The service probably persists this configuration.”
- **Why**: Explicit evidence limits let readers distinguish a verified absence from an unsupported guess.

## Worked Example: Evidence to README

The following is a **format example only**. The names and paths are illustrative; never copy them as facts unless the target repository independently verifies them.

**Evidence collected**: A public header declares `DeviceState` with `id` and `connected`; implementation code constructs and updates it in an in-memory map; no storage writes for that type were found in the reviewed call path.

**Appropriate README wording**:

```markdown
### Device state

`DeviceState` records a device identifier and its connection state
([type definition](../src/example/device_state.h)). The manager updates these
records in memory ([implementation](../src/example/device_manager.cpp)); the
reviewed path does not show persistent storage for this state.
```

**Inappropriate wording**:

```markdown
The device database persists every device state across restarts.
```

The second claim invents both a database and persistence. The example demonstrates the expected evidence-qualified style; replace the illustrative sources and facts with the target repository's verified evidence.

## Output Requirements

- **File Path**: `docs/README.md` at the target repository root. Ask the user if the target repository is ambiguous; do not silently choose a nested repository.
- **Persistent File Scope**: Modify only `docs/README.md`. A unique temporary tracker under `tmp/` is allowed during work and must be removed only if this task created it.
- **Format**: Markdown compatible with the repository's existing documentation renderer. Use fenced code blocks with appropriate language identifiers and valid, renderer-supported Mermaid where available.
- **Required Content**: responsibility and system boundary; C4 context/container/component views or explicit evidence-based explanation of missing views; grounded data-model summary; inbound API and outbound integration documentation; verified links to useful existing documentation.
- **Source Traceability**: Include relative links to relevant repository sources near non-obvious claims where practical. Do not add raw line-number citations that become misleading after normal edits.
- **Quality**: Accurate, concise, navigable, maintainable, internally consistent, and free of broken relative links or unsupported guarantees.
- **No Unrequested Outputs**: Do not create a separate architecture diagram, API document, tracking report, or code change.

---

# ⚠️ MANDATORY REPOSITORY DOCUMENTATION PROCESS ⚠️

**READ THIS SECTION COMPLETELY BEFORE STARTING WORK.** Complete each phase in sequence, update the research tracker at the end of each phase, and do not move forward until its validation criteria pass.

## Critical Process Rules

**YOU MUST:**

1. ✅ Work through Phases 0-7 in order and meet each phase's validation criteria before continuing.
2. ✅ Update the evidence tracker after every phase; do not rely on memory for source claims.
3. ✅ Cite or link repository evidence for architecture, model, and API facts.
4. ✅ Preserve accurate existing README material and limit persistent edits to `docs/README.md`.
5. ✅ Build or update the README incrementally by coherent sections, using multiple focused file edits rather than one enormous write.
6. ✅ Re-read changed sections and inspect the final diff before declaring completion.

**YOU MUST NOT:**

1. ❌ Draft the final README before completing evidence collection and consolidation.
2. ❌ Create/update the entire README in one operation if it is large; this risks truncation and omissions.
3. ❌ Skip the incremental approach: tracker sections must be recorded phase-by-phase, and README sections must be edited and verified separately.
4. ❌ Expand the task to code changes or additional persistent documents.
5. ❌ Hide uncertainty behind definitive wording or a success-shaped validation report.

**⚠️ OUTPUT-SIZE WARNING:** If the README is expected to reach 500 or more lines, use multiple focused edits (one or two coherent sections per edit), then re-read each edited range. Do not attempt to generate a 500+ line document in one tool operation. Keep this README concise; large catalogs belong in existing detailed references, not newly created files.

**Failure to follow the evidence, scope, or validation rules invalidates the documentation result.**

---

## PHASE 0: Create the Evidence Tracker

**⚠️ MANDATORY FIRST STEP — DO THIS BEFORE RESEARCH OR EDITING ⚠️**

Create a unique scratch file named `tmp/docs-readme-research-<task-id>.md`. Use a short unique ID; if the proposed path already exists, choose another. Do not modify or delete any pre-existing tracker.

Initialize it with:

```markdown
# docs/README.md Research Tracker

## 1. Repository and Documentation Map
Status: pending

## 2. Existing README Assessment
Status: pending

## 3. Architecture and C4 Evidence
Status: pending

## 4. Data Model Evidence
Status: pending

## 5. API and Integration Evidence
Status: pending

## 6. Consolidated README Plan
Status: pending

## 7. Validation Results
Status: pending
```

Record the exact tracker path so Phase 7 removes only that file after successful validation.

**Update the tracker**: The headings above must exist with `pending` status before any source analysis begins.

**REQUIRED OUTPUT**: A unique research tracker with all seven sections.

**Validation**: Before proceeding to Phase 1, verify the tracker exists, contains all seven headings, and was not created by overwriting an existing file.

## PHASE 1: Map the Repository and Existing Documentation

**⚠️ MANDATORY: Complete Tracker Sections 1-2 BEFORE proceeding to Phase 2 ⚠️**

### What to inspect

1. Confirm the repository root and inspect the top-level structure.
2. Read `docs/README.md` if it exists. Also inspect the root README, existing docs, developer instructions, and relevant build/configuration entry points.
3. Find the repository's Markdown/Mermaid conventions and identify available link targets. Do not assume a renderer supports Mermaid C4 syntax.
4. Identify likely authoritative headers, API definitions, schemas, model types, registration code, and integration call sites for Phases 2-3.

### What to record

- Repository root and component purpose as described by existing sources.
- README presence, sections, accurate content to preserve, and stale or unsupported content that needs correction.
- Existing documentation conventions, verified relative-link candidates, and relevant source paths.
- Which sources are authoritative for public APIs, types, configuration, runtime topology, and persistence.

**Update the tracker**:

- Set Sections 1-2 to `complete`.
- List inspected paths and a brief description of what each contributes.
- Record existing content to keep, revise, or remove with a reason; do not make README edits yet.

**REQUIRED OUTPUT**: A repository/documentation map and an explicit preservation plan for the README.

**Validation**: Before proceeding, verify both tracker sections contain concrete repository paths, the existing README was actually read if present, and likely primary sources for every requested documentation area are identified.

## PHASE 2: Trace Architecture and C4 Evidence

**⚠️ MANDATORY: Complete Tracker Section 3 BEFORE proceeding to Phase 3 ⚠️**

### What to trace

1. Find plugin/service entry points, registration, exported service identity, and initialization/lifecycle code.
2. Trace build and install metadata to distinguish libraries, plugins, processes, executables, and product-specific artifacts.
3. Trace representative calls/events to identify users, external systems, platform services, and communication direction.
4. Verify which dependencies are unconditional versus optional, compile-time gated, or dependent on a product/runtime configuration.
5. Group responsibilities into a small number of meaningful components inside actual runtime boundaries.

Do not infer a complete system landscape from a single call site. Mark an unknown actor, deployment host, or protocol as unknown if the repository does not establish it. Do not promote source directories or linked libraries to C4 containers without evidence they are independently runnable or deployable.

**Update the tracker Section 3** with:

```markdown
## 3. Architecture and C4 Evidence
Status: complete

### System context
- System boundary:
- Actors/external systems:
- Evidence paths:

### Containers/deployment units
- Unit and runtime responsibility:
- Communication paths:
- Optionality or configuration gates:
- Evidence paths:

### Internal components
- Component and responsibility:
- Dependencies/direction:
- Evidence paths:

### Unknowns and qualifications
- [Question or limitation; include evidence examined]

### Candidate diagram relationships
- [A] --[verified relationship/protocol]--> [B] (source: `path/to/file`)
```

**REQUIRED OUTPUT**: A verified, source-linked architecture inventory sufficient to draw three accurate C4 views without inventing boundaries.

**Validation**: Before proceeding, verify that every proposed node and relationship has an evidence path; runtime containers are distinguished from internal components; optionality is recorded; and unsupported assumptions are listed as unknowns rather than diagrammed as facts.

## PHASE 3: Trace Data Models and API Contracts

**⚠️ MANDATORY: Complete Tracker Sections 4-5 BEFORE proceeding to Phase 4 ⚠️**

### What to trace

1. Locate public model/type definitions, enums, configuration structures, serialization/deserialization, and event payloads.
2. Follow model use to establish relationships, ownership, lifecycle, and whether data is transient, configuration-backed, or persisted.
3. Locate formal inbound API definitions and verify service/callsign, protocol, version/revision, methods, events, payloads, and compatibility notes from definitions and registrations.
4. Trace outbound client calls separately. Identify the remote interface and dependency only as precisely as the code supports.
5. Compare real definitions with tests or mocks where both exist. Use mocks as corroboration, never as the sole source for a disputed API shape when an authoritative definition is available.
6. Record gaps explicitly. Do not claim a database, persistence guarantee, public method, or event because a type name or test fixture suggests one.

**Update the tracker Sections 4-5** with:

```markdown
## 4. Data Model Evidence
Status: complete
- Type/entity and source:
- Important fields/relationship:
- Configuration, serialization, or lifecycle evidence:
- Persistence mechanism (or "none found in reviewed code"):
- Confidence/limitations:

## 5. API and Integration Evidence
Status: complete
### Inbound/exposed contracts
- Interface, protocol, service identity, version/revision:
- Operations/events:
- Payload/model definitions:
- Registration and source paths:
- Compatibility or availability qualifications:

### Outbound calls and platform dependencies
- Called interface/dependency:
- Direction and purpose:
- Optionality/build/product conditions:
- Source paths:

### Unverified or absent details
- [Claim not established and relevant paths examined]
```

**REQUIRED OUTPUT**: A source-backed model inventory and a directionally separated list of inbound APIs and outbound integrations.

**Validation**: Before proceeding, verify each documented type and contract points to an authoritative source; persistence statements are backed by implementation; mock-only shapes are not presented as facts; and inbound and outbound interfaces cannot be confused.

## PHASE 4: Consolidate Evidence and Plan the README

**⚠️ MANDATORY: Complete Tracker Section 6 BEFORE creating or editing the README ⚠️**

### What to consolidate

1. Compare architecture, model, API, and existing-documentation notes for contradictions or duplicated claims.
2. For each proposed README claim, identify at least one source path. Remove or qualify claims without sufficient evidence.
3. Decide which detail belongs in the README and which should be linked to an existing authoritative reference.
4. Plan the headings, diagram nodes/edges, concise model summary, API subsections, and only verified documentation links.
5. List any relevant requested detail that could not be verified and the exact wording needed to avoid implying it exists.

**Update the tracker Section 6** with:

```markdown
## 6. Consolidated README Plan
Status: complete

### Sections and claims
- Section:
- Verified claim(s) and source path(s):
- Existing content to preserve or revise:

### Diagram plan
- Context:
- Containers:
- Components:
- Renderer/format choice and repository evidence:

### Model and API plan
- Models to summarize and source paths:
- Inbound contracts:
- Outbound dependencies:
- Known gaps/qualifications:

### Links
- README label -> verified repository-relative target:

### Exclusions
- [Tempting but unsupported claim/detail and reason for exclusion]
```

**REQUIRED OUTPUT**: A compact outline where every planned claim, diagram relationship, and link is either source-backed or explicitly qualified.

**Validation**: Before proceeding, verify all required README areas are planned, every non-obvious claim has a source, unknowns are qualified, the diagram format is compatible with repository conventions, and existing useful content is accounted for.

## PHASE 5: Create or Update docs/README.md

**⚠️ MANDATORY: Write only after Phase 4 validation succeeds ⚠️**

Create `docs/` only if needed. If `docs/README.md` exists, edit it in place and retain accurate useful content. Draft in bounded section-level edits; do not replace a large file in a single operation. After each coherent edit, read back the changed section and check it against the tracker.

Use this structural template, adapting headings only to preserve existing conventions:

```markdown
# [Component or Repository Name]

## Overview
[Purpose, responsibility boundary, and links to verified source/documentation.]

## Architecture
### System context
[C4 context diagram and concise explanation.]
### Containers
[C4 container diagram and concise explanation.]
### Components
[C4 component diagram and concise explanation.]

## Data models
[Key types and relationships; configuration/persistence only as verified.]

## APIs and integrations
### Inbound APIs
[Exposed contract, service identity, protocol/version, operations/events, model links.]
### Outbound integrations
[Interfaces called, purpose, and conditions/optional status.]

## Related documentation
[Only existing, verified repository links.]
```

If a layer cannot be represented accurately because evidence is missing, keep the corresponding heading and state the specific limitation. Do not create a fake diagram merely to fill a template.

For every diagram relationship, use a label justified by evidence. Every diagram needs a title and clear system boundary; optional integrations must be visibly marked optional or product-dependent. Prefer Mermaid C4 if supported. Otherwise use the project's supported renderer or readable text.

**Update the tracker**: Mark Section 6 as used for drafting; record the README sections written and any evidence-driven deviations. Do not mark validation results complete yet.

**REQUIRED OUTPUT**: A coherent `docs/README.md` containing verified architecture, data-model, and API documentation while retaining useful existing content.

**Validation**: Before proceeding, read the whole README, confirm every requested area is present or explicitly qualified, verify changed sections against tracker sources, and confirm no unrelated file was modified.

## PHASE 6: Validate and Review

**⚠️ MANDATORY: Complete all checks and correct failures BEFORE finalizing ⚠️**

### Required checks

1. **Factual coverage**: Recheck each non-obvious claim against the listed source. Verify product/build qualifiers and interface direction. Remove or qualify unsupported claims.
2. **C4 accuracy**: Check that context, container, and component levels are distinct; every actor, runtime unit, component, and relationship is supported; labels are consistent; optionality is explicit.
3. **Data-model accuracy**: Verify names and relationships against definitions. Ensure persistence/configuration statements are supported by actual serialization or storage code.
4. **API accuracy**: Verify service identity, protocol, revision/version, operations/events, and payload sources against authoritative definitions. Ensure outbound integrations are not described as APIs the component exposes.
5. **Markdown and links**: Inspect fences and headings; validate every relative link from `docs/README.md`. Use repository documentation tooling if already available and relevant. Do not install tools or introduce a dependency for this task.
6. **Content preservation**: Compare against the pre-edit README assessment; retain useful accurate content and avoid unnecessary deletion.
7. **Scope**: Review `git diff -- docs/README.md` and the full worktree status. Ensure no unrelated tracked or persistent file was changed by this task. Do not overwrite or revert changes that predated the task.
8. **Renderer**: If a supported local renderer/check exists, validate the Mermaid C4 blocks with it. If not, inspect syntax and state that rendering was not locally tested; do not claim it was.

If a check fails, correct the README and rerun the affected checks. Report an unavailable check as unavailable, not passed.

**Update the tracker Section 7** with:

```markdown
## 7. Validation Results
Status: complete | blocked
- Source claim review: pass/fail + corrections:
- C4 structure and evidence: pass/fail:
- Data models and API direction: pass/fail:
- Relative links: pass/fail + broken targets:
- Markdown/renderer checks: command/result or not available:
- Diff scope and content preservation: pass/fail:
- Remaining limitations:
```

**REQUIRED OUTPUT**: A verified README and honest, recorded validation outcomes.

**Validation**: Before proceeding, verify all required checks are either passed or explicitly marked unavailable/blocked with an explanation; fix all documentation errors within scope; and ensure the final diff contains only the requested README change.

## PHASE 7: Finalize and Report

**⚠️ MANDATORY: Finalize only after Phase 6 is complete ⚠️**

1. Confirm the final README is saved and the final diff still contains only `docs/README.md`.
2. Remove the exact unique research-tracker file created in Phase 0. Delete no other file or directory; if cleanup fails, report the exact path and reason.
3. Do not commit changes unless the user explicitly asked for a commit.
4. Report the README path, major sections created/updated, validations actually performed, and any evidence or renderer limitations. Keep the report concise and do not claim checks that were not run.

**Update the tracker**: Record the final summary and cleanup outcome before removing the scratch tracker. The persistent deliverable remains only `docs/README.md`.

**REQUIRED OUTPUT**: A concise completion summary grounded in the actual final diff and validation results.

**Validation**: Before reporting, verify `docs/README.md` exists, no tracker created by this task remains, and no out-of-scope persistent change was introduced.

---

# MANDATORY COMPLETION CHECKLIST

**Before considering this task complete, you MUST verify every applicable item below.**

## Phase Completion

- [ ] **Phase 0**: Created a unique tracker with all seven sections and did not overwrite an existing file.
- [ ] **Phase 1**: Mapped the repository and existing docs; assessed `docs/README.md` and recorded what to preserve.
- [ ] **Phase 2**: Collected source-backed system, container, component, relationship, and optionality evidence.
- [ ] **Phase 3**: Collected data-model evidence and separately documented inbound APIs and outbound integrations.
- [ ] **Phase 4**: Consolidated claims, sources, exclusions, links, and the README plan before editing.
- [ ] **Phase 5**: Created or updated only `docs/README.md`, including the requested architecture, models, and APIs.
- [ ] **Phase 6**: Reviewed facts, C4 distinctions, model/API direction, links, renderer/tool results, preservation, and diff scope.
- [ ] **Phase 7**: Removed only the task-created tracker and prepared an accurate completion report.

## Content Verification

- [ ] Responsibility boundary is supported by repository evidence.
- [ ] System context, container/deployment, and component architecture views are present or their evidence-based limitations are stated.
- [ ] Diagrams distinguish runtime units from source folders and identify optional integrations correctly.
- [ ] Model and relationship descriptions match authoritative definitions; persistence claims match implementation evidence.
- [ ] Inbound APIs and outbound calls are separate and accurately identify protocol, service identity, version, operations/events, and payload sources where available.
- [ ] Existing useful README content is preserved and inaccurate content is corrected with evidence.
- [ ] Related links point only to existing, verified repository targets.

## Quality and Scope Verification

- [ ] Every non-obvious claim has a traceable source or a clear qualification.
- [ ] Markdown fences/headings and supported diagram syntax are valid; unavailable rendering checks are disclosed.
- [ ] Relative Markdown links resolve from `docs/README.md`.
- [ ] The persistent diff contains no source, build, test, schema, or unrelated documentation changes.
- [ ] No unsupported guarantees, fabricated API details, credentials, or production secrets were added.
- [ ] Validation results report only checks that were actually performed.

## Process Verification

- [ ] Did NOT skip any phase or draft before consolidating evidence.
- [ ] Updated the tracker after each phase and verified each coherent README edit.
- [ ] Did NOT overwrite an existing tracker or delete any file not created for this task.
- [ ] Did NOT create extra persistent documents or modify application code.
- [ ] Did NOT claim an unavailable test or renderer check passed.

**IF ANY CHECKBOX ABOVE IS UNCHECKED, THE TASK IS NOT COMPLETE. If a criterion is genuinely inapplicable, document why in the tracker and completion summary rather than silently skipping it.**

## Success Criteria

This task is successful when:

1. **Completeness**: `docs/README.md` addresses architecture, data models, APIs, and relevant existing documentation or states evidence-based limitations.
2. **Accuracy**: Every substantive claim matches repository evidence; unknowns are explicit.
3. **Architectural clarity**: Readers can distinguish system context, runtime/deployment units, internal components, inbound contracts, and outbound dependencies.
4. **Usability**: Developers can navigate the page and follow verified links to authoritative details.
5. **Preservation**: Useful existing README information remains, with changes limited to requested documentation.
6. **Validation**: Link, syntax, source, and diff checks are honestly recorded and all discovered in-scope issues are corrected.
