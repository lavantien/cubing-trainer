$ErrorActionPreference = "Continue"
Set-Location "C:\Users\lavantien\dev\github\cubing-trainer"
$stamp = "playground/gates/verify2_progress.txt"
"$(Get-Date -Format o) verify2 lane 4 resume pid $PID journal kept" | Add-Content $stamp
$suites = "e2e_data_dir_override,e2e_parse_memo_words,e2e_parse_shot_words,e2e_run_expect_genalg,e2e_run_memo_guards,e2e_run_memo_roundtrip,e2e_run_script_guards,e2e_run_trainer_edge_generate,e2e_run_trainer_generate,e2e_run_trainer_memo_panel,trainer_decomp_lines,trainer_form_all_lists_counts,trainer_form_case_detail,trainer_form_cell_pair_overflow,trainer_form_chart_cells,trainer_form_generated_alg,trainer_form_letter_scheme_routed,trainer_form_memo_find_rows,trainer_form_memo_lifecycle,trainer_form_memo_save_guards,trainer_form_navigation,trainer_form_open,trainer_form_select_refreshes_inputs,trainer_form_visible_count_edges,trainer_screen_update_reopen"
make BUILDDIR=build_gate mutate SCOPE=src/ui/screen_trainer.c "SUITE=$suites" 1>> playground/gates/verify2_4_screen_trainer.log 2>> playground/gates/verify2_4_screen_trainer.err
"$(Get-Date -Format o) verify2 lane 4 src/ui/screen_trainer.c EXIT=$LASTEXITCODE" | Add-Content $stamp
"$(Get-Date -Format o) verify2 orchestrator DONE" | Add-Content $stamp
