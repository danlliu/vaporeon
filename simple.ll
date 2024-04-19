; ModuleID = 'simple.c'
source_filename = "simple.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

@.str = private unnamed_addr constant [20 x i8] c"Buffer content: %s\0A\00", align 1
@.str.1 = private unnamed_addr constant [19 x i8] c"Enter some input: \00", align 1
@stdin = external global ptr, align 8

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local void @sensitiveFunction(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  %3 = alloca [50 x i8], align 16
  store ptr %0, ptr %2, align 8
  %4 = getelementptr inbounds [50 x i8], ptr %3, i64 0, i64 0
  %5 = load ptr, ptr %2, align 8
  %6 = call ptr @strcpy(ptr noundef %4, ptr noundef %5) #3
  %7 = getelementptr inbounds [50 x i8], ptr %3, i64 0, i64 0
  %8 = call i32 (ptr, ...) @printf(ptr noundef @.str, ptr noundef %7)
  ret void
}

; Function Attrs: nounwind
declare ptr @strcpy(ptr noundef, ptr noundef) #1

declare i32 @printf(ptr noundef, ...) #2

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local void @processInput(ptr noundef %0) #0 {
  %2 = alloca ptr, align 8
  %3 = alloca [100 x i8], align 16
  store ptr %0, ptr %2, align 8
  %4 = getelementptr inbounds [100 x i8], ptr %3, i64 0, i64 0
  %5 = load ptr, ptr %2, align 8
  %6 = call ptr @strcpy(ptr noundef %4, ptr noundef %5) #3
  %7 = getelementptr inbounds [100 x i8], ptr %3, i64 0, i64 0
  call void @sensitiveFunction(ptr noundef %7)
  ret void
}

; Function Attrs: noinline nounwind sspstrong uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  %2 = alloca [100 x i8], align 16
  store i32 0, ptr %1, align 4
  %3 = call i32 (ptr, ...) @printf(ptr noundef @.str.1)
  %4 = getelementptr inbounds [100 x i8], ptr %2, i64 0, i64 0
  %5 = load ptr, ptr @stdin, align 8
  %6 = call ptr @fgets(ptr noundef %4, i32 noundef 100, ptr noundef %5)
  %7 = getelementptr inbounds [100 x i8], ptr %2, i64 0, i64 0
  call void @processInput(ptr noundef %7)
  ret i32 0
}

declare ptr @fgets(ptr noundef, i32 noundef, ptr noundef) #2

attributes #0 = { noinline nounwind sspstrong uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 17.0.6"}
