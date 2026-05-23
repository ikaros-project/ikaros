# devlog Writing Guide

This guide describes how to write weekly and yearly Ikaros devlog entries so future entries match the existing style.

## Core Principles

- Write from the standpoint of the week or year being described. Do not use knowledge of later development to explain why a change mattered.
- Use calendar weeks, ISO week numbering, and omit weeks with no repository activity.
- Keep the style narrative-first. The logs should read like a development history, not a raw commit digest.
- Include contributor credit in prose, using names or account names as recorded in Git, but do not include email addresses.
- Do not list every commit. Mention the initial commit for a week when relevant, then focus on major changes, integrations, fixes, or shifts in direction.
- Prefer direct, authoritative language. Write "the week is a transition" rather than "the week reads like a transition."
- Preserve uncertainty only when the Git history is genuinely ambiguous.

## Weekly Entries

Weekly entries live in the year folder:

```text
devlog/YYYY/YYYY-MM-DD-week-NN.md
```

Use the Monday date of the ISO week in the filename. Number entries sequentially within the year, skipping inactive weeks.

Each weekly entry should use this structure:

```markdown
# Title

**Week:** YYYY-WNN  
**Date range:** YYYY-MM-DD to YYYY-MM-DD  
**Commit range:** `full-first-hash` to `full-last-hash`  
**Status:** Draft for review

## Week in Brief

One short paragraph summarizing the active week.

## Highlights

- Major change.
- Major update.
- Important cleanup.

## Development Notes

One or more paragraphs that explain the week as a coherent development moment.
Mention contributors in context.
Avoid future knowledge.

## Major Commits And Updates

- `shortsha` - Major commit or grouped update.

## State At Week's End

- What was true at the end of this week.
- What had become more stable, usable, or available.
```

### Weekly Style Rules

- Keep single-commit weeks short.
- Give dense weeks more narrative detail.
- If a commit subject contains awkward wording, keep the subject in commit lists but write cleaner prose in summaries.
- It is acceptable to refer to earlier work that already existed by that week.
- Avoid phrases like "setting up the work that came next," "prepared for later," or "this would become important." Instead, say what changed by the end of the week.
- Newly generated weekly entries should be `Draft for review`. Change to `Approved` only after review.

## Yearly Summaries

Each year folder has a yearly summary at:

```text
devlog/YYYY/README.md
```

The yearly README should summarize that year in longer narrative form and include an index of that year's weekly entries at the end.

Use this structure:

```markdown
# Ikaros YYYY: Year Title

**Year:** YYYY  
**Status:** Draft for review  
**Suggested visual:** One screenshot or image suggestion.

## Year in Brief

Two to four sentences summarizing the year from the standpoint of that year.

## Development Notes

Several paragraphs organized by themes or phases within the year.
Mention contributors in context when useful.
Do not refer to later years or future outcomes.

## Milestones

- Curated major milestone.
- Curated major milestone.

## Weekly Entries

| Week | Date range | Title |
| --- | --- | --- |
| YYYY-WNN | YYYY-MM-DD to YYYY-MM-DD | [Title](YYYY-MM-DD-week-NN.md) |
```

### Yearly Style Rules

- Yearly summaries should be more narrative and interpretive than weekly entries.
- Do not include a "Looking Forward" section.
- Suggested visuals should be concrete: a WebUI screenshot, module output, simulation view, plot, or image-processing result.
- Make each prose paragraph substantial enough to stand alone. Explain why the year's work mattered within that year, not because of later developments.
- Keep milestone lists curated and short.

## Main Index

The main index is:

```text
devlog/README.md
```

It should contain:

- A short explanation of the devlog conventions.
- A `Yearly Summaries` table linking to each `YYYY/README.md`.
- A full `Entries` table linking to every weekly entry.

When adding new entries, update both:

- `devlog/README.md`
- `devlog/YYYY/README.md`

## Contributor Credit

Use Git author names or account names, without email addresses.

Good:

```text
Birger Johansson updated the CMake support.
Work recorded under `ikaros-project` added the WebUI changes.
Christian Balkenius completed the trainer work.
```

Avoid:

```text
Birger Johansson <email address>
```

If the same contributor appears under several account names, mention the real name when it is clear, and mention account variants only when the distinction matters historically.

## Date Perspective

Always write from the date being described.

Good:

```text
By the end of the week, the WebUI had persistent view state.
```

Avoid:

```text
This prepared the WebUI for the reconnect work that would come later.
```

Good:

```text
The year closes with a stronger video path, broader tests, and a more consistent module/runtime surface.
```

Avoid:

```text
The year was a runway for the next year's UI and safety work.
```

## Verification Checklist

Before finishing a devlog update:

- Confirm active weeks from Git history.
- Confirm inactive weeks are omitted.
- Confirm filenames and ISO week numbers match.
- Confirm no email addresses appear in devlog text.
- Confirm weekly entries have the intended status.
- Confirm yearly summaries do not contain future-looking hindsight.
- Confirm all Markdown links resolve.
- Confirm existing approved entries are not accidentally changed back to draft.
